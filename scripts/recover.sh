#!/bin/bash
git fetch origin
git reset --hard origin/main
git clean -fd
