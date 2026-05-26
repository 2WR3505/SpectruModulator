# SpectruModulator
A software spectrum waterfall image generator specifically for FM broadcast transmitters that converts images to sound in wav files.

Use this with an actual transmitter, not a short range bluetooth adapter

# Compiling

First get the dependency of stb_image as its needed to compie.

To compile, just use the simple gcc command here: gcc -O2 -o SpectruModulator SpectruModulator-V0.1-alpha.c -lsndfile -lm

# Usage

./SpectruModulator image.png image.wav

The programs default time is 50 seconds of audio, for better clarity use the following

./SpectruModulator image.png image.wav 90
