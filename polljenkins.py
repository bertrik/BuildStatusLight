#!/usr/bin/env python3

"""
   Polls a Jenkins build server for status over HTTP and
   pushes the result over serial to a toy traffic light.
"""

import time
import argparse
import requests
import serial

class JenkinsPoller():
    """ Polls build status from HTTP, pushes a command over serial """

    def __init__(self, url, serialport, interval):
        self.url = url
        self.serialport = serialport
        self.interval = interval

    def run(self):
        """ runs the main process of this class """
        with serial.Serial(self.serialport, 115200) as ser:
            while True:
                # get the build result from Jenkins build status JSON
                print(f'Polling build status from {self.url}')
                try:
                    response = requests.get(self.url)
                    json = response.json()
                    result = json['result']
                except Exception:
                    print('GET failed!')
                    result = None

                # parse it
                print(f"> build result: {result}")
                if result == 'FAILURE':
                    # red
                    cmd = 'set 1\n'
                elif result == 'UNSTABLE':
                    # orange/yellow
                    cmd = 'set 2\n'
                elif result == 'SUCCESS':
                    # green
                    cmd = 'set 3\n'
                else:
                    # flashing
                    cmd = 'set 4\n'

                # send it to serial
                print(f'> send serial command:', cmd.strip())
                ser.write(cmd.encode('ascii'))

                # wait until next poll
                print(f'> sleeping {self.interval} seconds until next poll')
                time.sleep(self.interval)

def main():
    """ The main entry point """

    # you can run a local HTTP server serving files
    # from the current local directory on port 8000, using
    #   python3 -m http.server
    # then use an url parameter of 'http://localhost:8000/jenkins.json'

    parser = argparse.ArgumentParser()
    parser.add_argument("-u", "--url", help="The URL of Jenkins build result JSON",
                        default="http://jenkins/view/MobiMaestro/job/MM_master_Nightly/lastBuild/api/json")
    parser.add_argument("-s", "--serialport", help="The serial port to write to", default="/dev/ttyUSB0")
    parser.add_argument("-i", "--interval", help="The poll interval (seconds)", default=60)
    args = parser.parse_args()

    poller = JenkinsPoller(args.url, args.serialport, args.interval)
    poller.run()

if __name__ == "__main__":
    main()
