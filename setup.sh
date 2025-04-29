#!/bin/bash

sudo apt install libxml2-dev
wget https://download.geofabrik.de/north-america/us/new-york-latest.osm.bz2
bzip2 -d new-york-latest.osm.bz2