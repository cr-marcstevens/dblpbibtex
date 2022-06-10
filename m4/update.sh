#!/bin/bash

for f in ax_*.m4; do 
	wget -O ${f} "https://git.savannah.gnu.org/gitweb/?p=autoconf-archive.git;a=blob_plain;f=m4/$f"
done
