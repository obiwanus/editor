#!/bin/bash

DROPBOX_DIR=$HOME/Dropbox/projects
WORKING_DIR_NAME=${PWD##*/}
WORKING_DIR=$PWD

echo "Saving the contents of $WORKING_DIR"

rsync -avzh --exclude '.git/*' --delete-excluded $WORKING_DIR $DROPBOX_DIR/$WORKING_DIR_NAME