#!/usr/bin/env python3
# coding: utf-8

import argparse
import subprocess
import os
import ips
import sys
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input", dest="str_firmwareFolder", metavar="FOLDERPATH", help="Path to firmware files.", default="registered")
parser.add_argument("-o", "--output", dest="str_patchesFolder", metavar="FOLDERPATH",
                    help="Path to output IPS patches to.", default="VI_Patches")

args_arguments = parser.parse_args()

# Module we're looking for.
str_moduleName = "vi"

# String of hex values to search for in the file.
str_searchSequence = "00 04 40 39 C0 03 5F D6"

# This is what replaces the string above
str_newHex = "20 00 80 52"

# This is stripped from the title of the IPS
str_stripFromIPS = "000000000000000000000000"


def find_in_text(text, find) -> str:
    for i in text.split("\\r\\n"): # Note: Line endings need to be changed according to OS! Windows = \r\n, Linux = \n
        if find in i:
            return i.split(":")[1].strip()


def main():
    if not os.path.exists(args_arguments.str_firmwareFolder):
        print("Error: firmware folder doesn't exist.")
        return -1

    # Ensure output folder exists.
    if not os.path.exists(args_arguments.str_patchesFolder):
        os.makedirs(args_arguments.str_patchesFolder)

    print(f"Input firmware folder: {args_arguments.str_firmwareFolder}")
    print(f"IPS Patches folder: {args_arguments.str_patchesFolder}")

    int_fileCount = 0

    for currentFile in os.listdir(args_arguments.str_firmwareFolder):

        str_filePath = os.fsdecode(os.path.join(
            args_arguments.str_firmwareFolder, currentFile))

        if not os.path.isfile(str_filePath):
            continue

        str_fileExtension = os.path.splitext(str_filePath)[1]
        if str_fileExtension.lower() != ".nca":
            continue

        int_fileCount += 1
        print(f"Processing {int_fileCount} : {str_filePath}", end="\r")

        str_title = find_in_text(str(subprocess.check_output(["./hactool", "--disablekeywarns", "-i", f"{str_filePath}"])), "Title Name:")
        if not str_title == str_moduleName:
            continue

        print(f"NCA found: {str_filePath}. Processing...")

        str_tempFolder = os.fsdecode(str_filePath + "_out")
        str_main = os.fsdecode(os.path.join(str_tempFolder, "main"))
        str_uncompressedMain = os.fsdecode(os.path.join(str_tempFolder, "main_uncompressed"))

        subprocess.call(["./hactool", "--disablekeywarns", "-t", "nca",
                        "--exefsdir", f"{str_tempFolder}", f"{str_filePath}"])
        str_output = str(subprocess.check_output(["./hactool", "--disablekeywarns",
                                                  f"--uncompressed={str_uncompressedMain}", "-t", "nso0", f"{str_main}"]))

        str_buildID = find_in_text(str_output, "Build Id: ")
        print(f"Title name: {str_title}")
        print(f"Build ID {str_buildID}")

        bin_sequence = bytes.fromhex(str_searchSequence)
        with open(f"{str_uncompressedMain}", "rb") as file_Main:
            # I'm assuming this reads the entire file at once?
            bin_fileContents = file_Main.read()
            try:
                # I'm assuming this searches the file buffer for the sequence?
                int_hexOffset = bin_fileContents.index(bin_sequence)
                # Print it was found
                print("Sequence found at offset: " + hex(int_hexOffset))

                # Create ips patch
                ips_patch = ips.Patch()
                ips_patch.ips32 = False
                bin_patchData = bytes.fromhex(str_newHex)
                ips_patch.add_record(int_hexOffset, bin_patchData)

                # Write IPS file
                str_ipsPatchPath = os.fsdecode(os.path.join(args_arguments.str_patchesFolder, str_buildID.replace(str_stripFromIPS, "") + ".ips"))

                print(f"IPS patch written to: {str_ipsPatchPath}")

                # Open IPS file and write out the patch.
                with open(f"{str_ipsPatchPath}", "wb") as file_ipsFile:
                    file_ipsFile.write(bytes(ips_patch))

            except ValueError:
                print("Error creating IPS patch.")
                return -2

        return 0


if __name__ == "__main__":
    sys.exit(main())