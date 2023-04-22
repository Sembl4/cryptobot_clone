#! /bin/bash

cp -r ../research_data/* .
cp -r ../configs/* .
make my_project -j4
