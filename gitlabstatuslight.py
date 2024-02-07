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

    def __init__(self, host):
        self.host = host

    def get_latest_pipeline(self, projectid, token, ref):
        url = f"https://{self.host}/api/v4/projects/{projectid}/pipelines/latest"
        headers = {"PRIVATE-TOKEN" : token}
        params = {"ref": ref}
        response = requests.get(url, headers = headers, params = params, timeout = 20)
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

    def __init__(self, host):
        self.host = host

    def send_control(self, params):
        url = f"http://{self.host}/api/control"
        requests.get(url, params = params, timeout = 20)

    def set_color(self, colors):
        code = ""
        for tier in range(0, 5):
            color = colors[tier] if tier < len(colors) else PatliteClient.Color.OFF
            code = code + f"{color.value}"
        params = { "color" : code, "buzzer" : 0 }
        self.send_control(params)

def main():
    """ The main entry point """

    # you can run a local HTTP server acting as a patlite, run
    #   python3 -m http.server

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-gitlab", help="The gitlab instance host name",
        default="gitlab.technolution.nl")
    parser.add_argument("-t", "--token", help="The gitlab API private token", default="")
    parser.add_argument("-i", "--projectid", help="The gitlab project id", default="197")
    parser.add_argument("-patlite", help="The patlist host name", default = "localhost:8000")
    args = parser.parse_args()

    gitlab = GitlabCLient(args.gitlab)
    patlite = PatliteClient(args.patlite)
    refs = ["master", "releases/1.57.x"]
    while True:
        colors = []
        for ref in refs:
            result = gitlab.get_latest_pipeline(args.projectid, args.token, ref)
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
        patlite.set_color(colors)

        time.sleep(300)

if __name__ == "__main__":
    main()
