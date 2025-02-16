#!/usr/bin/env python3

"""
   Polls a gitlab CI server for build status and shows the result on a stack light.
"""

from enum import Enum

import time
import argparse
import requests


class GitlabCLient():
    """ Retrieves pipeline build status from gitlab """

    def __init__(self, host : str, projectid: str, token: str):
        self.host = host
        self.projectid = projectid
        self.token = token

    def get_latest_pipeline(self, ref: str):
        url = f"https://{self.host}/api/v4/projects/{self.projectid}/pipelines/latest"
        headers = {"PRIVATE-TOKEN": self.token}
        params = {"ref": ref}
        response = requests.get(url, headers=headers, params=params, timeout=20)
        if not response.ok:
            print(f"GET of {url} for ref '{ref}' failed: {response.status_code} - {response.reason}")
        return response.json()

    def get_jobs(self):
        url = f"https://{self.host}/api/v4/projects/{self.projectid}/jobs"
        headers = {"PRIVATE-TOKEN" : self.token}
        response = requests.get(url, headers=headers, timeout=20)
        if not response.ok:
            print(f"GET of {url} failed: {response.status_code} - {response.reason}")
        return response.json()

class PatliteClient():
    """ Communicates with a Patlite LA6 """

    class Color(Enum):
        """ Patlite color codes """
        OFF = 0
        RED = 1
        AMBER = 2
        LEMON = 3
        GREEN = 4
        SKYBLUE = 5
        BLUE = 6
        PURPLE = 7
        PINK = 8
        WHITE = 9

    def __init__(self, host: str):
        self.host = host

    def send_control(self, params):
        url = f"http://{self.host}/api/control"
        requests.get(url, params=params, timeout=20)

    def set_color(self, colors):
        code = ""
        for tier in range(0, 5):
            color = colors[tier] if tier < len(colors) else PatliteClient.Color.OFF
            code = code + f"{color.value}"
        params = {"color": code, "buzzer": 0}
        self.send_control(params)

def queue_duration_to_color(jobs) -> PatliteClient.Color:
    if len(jobs) == 0:
        return PatliteClient.Color.BLUE
    queued_duration = float(jobs[0]["queued_duration"])
    print(f"Most recent queued duration = {queued_duration}")
    if queued_duration < 20:
        return PatliteClient.Color.GREEN
    if queued_duration < 120:
        return PatliteClient.Color.AMBER
    return PatliteClient.Color.RED

def main():
    """ The main entry point """

    # you can run a local HTTP server acting as a patlite, run
    #   python3 -m http.server

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-gitlab", help="The gitlab instance host name",
                        default="gitlab.technolution.nl")
    parser.add_argument("-i", "--projectid", help="The gitlab project id", default="197")
    parser.add_argument("-t", "--token", help="The gitlab API private token", default="")
    parser.add_argument("-patlite", help="The patlist host name", default="localhost:8000")
    args = parser.parse_args()

    gitlab = GitlabCLient(args.gitlab, args.projectid, args.token)
    patlite = PatliteClient(args.patlite)
    refs = ["master", "releases/1.58.x", "releases/1.56.x"]
    while True:
        colors = []

        # build status
        for ref in refs:
            result = gitlab.get_latest_pipeline(ref)
            status = result.get("status", "undefined")
            match status:
                case "success":
                    color = PatliteClient.Color.GREEN
                case "failed":
                    color = PatliteClient.Color.RED
                case _:
                    color = PatliteClient.Color.BLUE
            print(f"{ref}: {status} -> {color}")
            colors.append(color)

        # job queue status
        jobs = gitlab.get_jobs()
        colors.append(queue_duration_to_color(jobs))

        patlite.set_color(colors)

        time.sleep(300)


if __name__ == "__main__":
    main()
