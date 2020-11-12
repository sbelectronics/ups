#!/usr/bin/python -u

import argparse
import daemon
import os
import signal
import sys
import time
import traceback

from smbpi.ups import UPS, diags


glo_Usr1Received = False
glo_HupReceived = False


def parse_args():
    parser = argparse.ArgumentParser()

    defs = {"pigpio": True,
            "diags": False,
            "showErrors": False,
            "time": None,
            "outputFile": None,
            "outputAppend": False,
            "shutdownCommand": "/sbin/shutdown -h now",
            "threshold": None,
            "daemon": False}

    _help = 'Delay in seconds (default: %s)' % defs['time']
    parser.add_argument(
        '-t', '--time', dest='time', action='store',  type=int,
        default=defs['time'],
        help=_help)

    _help = 'Do not use pigpio (default: %s)' % defs['pigpio']
    parser.add_argument(
        '-N', '--nopigpio', dest='pigpio', action='store_false',
        default=defs['pigpio'],
        help=_help)

    _help = 'Enter diags mode after command (default: %s)' % defs['diags']
    parser.add_argument(
        '-D', '--diags', dest='diags', action='store_true',
        default=defs['diags'],
        help=_help)

    _help = 'Show errors after command (default: %s)' % defs['diags']
    parser.add_argument(
        '-e', '--errors', dest='showErrors', action='store_true',
        default=defs['showErrors'],
        help=_help)

    _help = 'Send output to file (default: %s)' % defs['outputFile']
    parser.add_argument(
        '-O', '--outputfile', dest='outputFile', action='store', type=str,
        default=defs['outputFile'],
        help=_help)

    _help = 'Append to log file(default: %s)' % defs['outputAppend']
    parser.add_argument(
        '-A', '--append', dest='outputAppend', action='store_true',
        default=defs['outputAppend'],
        help=_help)

    _help = 'Shutdown command (default: %s)' % defs['shutdownCommand']
    parser.add_argument(
        '-s', '--shutdownCommand', dest='shutdownCommand', action='store', type=str,
        default=defs['shutdownCommand'],
        help=_help)

    _help = 'Threshold on VIN to begin shutdown (Default: use uController setting)'
    parser.add_argument(
        '-T', '--threshold', dest='threshold', action='store', type=float,
        default=defs['threshold'],
        help=_help)

    _help = 'Daemonize (default: %s)' % defs['daemon']
    parser.add_argument(
        '-d', '--daemon', dest='daemon', action='store_true',
        default=defs['daemon'],
        help=_help)

    parser.add_argument('cmd', help='command')

    args = parser.parse_args()

    return args


def shutdown(ups, time):
    if time is None:
        time = 1000
    ups.setCountdown(time)


def show(ups):
    print "Using pigpiod: %s" % (ups.pi!=None)
    print "Using smbus: %s" % (ups.bus!=None)
    print "03 VIN: %0.2f" % ups.readVIN()
    print "05 VUPS: %0.2f" % ups.readVUPS()
    print "06 Mosfet: %0d" % ups.readMosfet()
    print "07 OnThresh: %0.2f" % ups.readOnThresh()
    print "08 OffThresh: %0.2f" % ups.readOffThresh()
    print "09 Countdown: %0.2d" % ups.readCountdown()
    print "0A State: %s" % ups.readStateStr()
    print "0D RunCounter: %d" % ups.readRunCounter()
    print "0E PowerUpThresh %0.2f" % ups.readPowerUpThresh()
    print "0F ShutdownThresh: %0.2f" % ups.readShutdownThresh()
    print "10 R1: %d" % (ups.readR1() * 100)
    print "11 R2: %d" % (ups.readR2() * 100)


def showErrors(ups):
    print "Errors: %d" % ups.errorCount
    print "Success: %d" % ups.errorSuccess
    print "IO: %d" % ups.errorIO
    print "rCrc: %d" % ups.errorReceiveCRC
    print "rSize: %d" % ups.errorReceiveSize
    print "rUninit: %d" % ups.errorReceiveUninitialized
    print "CRC: %d" % ups.errorCRC


def signal_hup(sig, stack):
    global glo_HupReceived
    glo_HupReceived = True


def signal_usr1(sig, stac):
    global glo_Usr1Received
    glo_Usr1Received = True


def monitor(ups, shutdownCommand, threshold):
    global glo_Usr1Received, glo_HupReceived

    signal.signal(signal.SIGHUP, signal_hup)
    signal.signal(signal.SIGUSR1, signal_usr1)
    print "starting monitor"
    while True:
        try:
            VIN = ups.readVIN()
            if (VIN < threshold):
                print "VIN of %0.2f is less than threshold of %0.2f" % (VIN, threshold)
                print "Executing shutdown command %s" % shutdownCommand
                os.system(shutdownCommand)

            if glo_Usr1Received:
                glo_Usr1Received = False
                show(ups)
                showErrors(ups)
                
            if glo_HupReceived:
                glo_HupReceived = False
                print "HUP Received"
                print "Executing shutdown command %s" % shutdownCommand
                os.system(shutdownCommand)
        except:
            traceback.print_exc()

        time.sleep(1)


def  execute(args):
    pi = None
    if args.pigpio:
        try:
            import pigpio
            pi = pigpio.pi()
            if pi.connected:
                ups = UPS(pi=pi)
            else:
                print "Failed to connect to pigpio; defaulting to smbus"
                pi = None
        except ImportError:
            print "Failed to import pigpio; defaulting to smbus"
            pi = None

    if not pi:
        import smbus
        bus = smbus.SMBus(1)
        ups = UPS(bus=bus)

    if args.cmd == "shutdown":
        shutdown(ups, args.time)
    elif args.cmd == "diags":
        args.diags = True
    elif args.cmd == "show":
        show(ups)
    elif args.cmd == "monitor":
        if args.threshold is None:
            threshold = ups.readShutdownThresh()
        else:
            threshold = args.threshold
        monitor(ups, args.shutdownCommand, threshold)
    else:
        raise Exception("Unknown command %s" % args.cmd)

    if args.diags:
        diags(ups)

    if args.showErrors:
        showErrors(ups)


def main():
    args = parse_args()

    if args.outputFile is not None:
        if args.outputAppend:
            outputFile = open(args.outputFile, "a", 0)
        else:
            outputFile = open(args.outputFile, "w", 0)
    else:
        outputFile = sys.stdout

    if args.daemon:
        context = daemon.DaemonContext(stdout=outputFile)
        with context:
            execute(args)
    else:
        sys.stdout = outputFile
        execute(args)


if __name__ == "__main__":
    main()
