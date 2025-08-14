#!/usr/bin/env python3
# coding: utf-8

import argparse
import subprocess
import os
import ips
import sys
from pathlib import Path

ARG_PARSER = argparse.ArgumentParser()
ARG_PARSER.add_argument("-i", "--input", dest="firmwareFolder", metavar="FOLDERPATH", help="Path to firmware files.", default="registered")
ARG_PARSER.add_argument("-o", "--output", dest="patchesFolder", metavar="FOLDERPATH",
                    help="Path to output IPS patches to.", default="vi_patches")

# Command line arguments.
ARGUMENTS = ARG_PARSER.parse_args()

# Module we're looking for.
MODULE_NAME: str = "vi"

# String of hex values to search for in the file.
SEARCH_SEQUENCE: str = "00 04 40 39 C0 03 5F D6"

# This is what replaces the string above
NEW_HEX: str = "20 00 80 52"

# This is stripped from the title of the IPS
STRIP_FROM_IPS: str = "000000000000000000000000"


def find_in_text(text: str, find: str) -> str:
    lineEndings: str
    if os.name == "nt":
        lineEndings = "\\r\\n";
    else:
        lineEndings = "\\n"

    for i in text.split(lineEndings):
        if find in i:
            return i.split(":")[1].strip()


def main():
    if not os.path.exists(ARGUMENTS.firmwareFolder):
        print("Error: firmware folder doesn't exist.")
        return -1

    # Ensure output folder exists.
    if not os.path.exists(ARGUMENTS.patchesFolder):
        os.makedirs(ARGUMENTS.patchesFolder)

    print(f"Input firmware folder: {ARGUMENTS.firmwareFolder}")
    print(f"IPS Patches folder: {ARGUMENTS.patchesFolder}")

    fileCount: int = 0
    for currentFile in os.listdir(ARGUMENTS.firmwareFolder):

        filePath: str = os.fsdecode(os.path.join(
            ARGUMENTS.firmwareFolder, currentFile))

        if not os.path.isfile(filePath):
            continue

        fileExtension: str = os.path.splitext(filePath)[1]
        if fileExtension.lower() != ".nca":
            continue

        fileCount += 1
        print(f"Processing {fileCount} : {filePath}", end="\r")

        title: str = find_in_text(str(subprocess.check_output(["./hactool", "--disablekeywarns", "-i", f"{filePath}"])), "Title Name:")
        if not title == MODULE_NAME:
            continue

        print(f"NCA found: {filePath}. Processing...")

        tempFolder: str = os.fsdecode(filePath + "_out")
        main: str = os.fsdecode(os.path.join(tempFolder, "main"))
        uncompressedMain: str = os.fsdecode(os.path.join(tempFolder, "main_uncompressed"))

        subprocess.call(["./hactool", "--disablekeywarns", "-t", "nca",
                        "--exefsdir", f"{tempFolder}", f"{filePath}"])
        output: str = str(subprocess.check_output(["./hactool", "--disablekeywarns",
                                                  f"--uncompressed={uncompressedMain}", "-t", "nso0", f"{main}"]))

        buildID: str = find_in_text(output, "Build Id: ")
        print(f"Title name: {title}")
        print(f"Build ID {buildID}")

        sequence: bin = bytes.fromhex(SEARCH_SEQUENCE)
        with open(f"{uncompressedMain}", "rb") as mainFile:
            # I'm assuming this reads the entire file at once?
            fileContents: bin = mainFile.read()
            try:
                # I'm assuming this searches the file buffer for the sequence?
                hexOffset: int = fileContents.index(sequence)
                # Print it was found
                print("Sequence found at offset: " + hex(hexOffset))

                # Create ips patch
                patch = ips.Patch()
                patch.ips32 = False
                patchData: bytes = bytes.fromhex(NEW_HEX)
                patch.add_record(hexOffset, patchData)

                # Write IPS file
                ipsPatchPath: str = os.fsdecode(os.path.join(ARGUMENTS.patchesFolder, buildID.replace(STRIP_FROM_IPS, "") + ".ips"))

                print(f"IPS patch written to: {ipsPatchPath}")

                # Open IPS file and write out the patch.
                with open(f"{ipsPatchPath}", "wb") as ipsFile:
                    ipsFile.write(bytes(patch))

            except ValueError:
                print("Error creating IPS patch.")
                return -2

        return 0


if __name__ == "__main__":
    sys.exit(main())