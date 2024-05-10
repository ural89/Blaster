#!/bin/bash

# Get the directory of the script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Function to delete folders
delete_folders() {
    local folders=("Binaries" "Intermediate" "Saved")

    for folder in "${folders[@]}"; do
        folder_path="${SCRIPT_DIR}/${folder}"
        if [ -d "$folder_path" ]; then
            echo "Deleting $folder_path"
            rm -rf "$folder_path"
        else
            echo "Folder $folder_path not found."
        fi
    done
}

# Main script starts here
delete_folders

