# OhSillyScope
For any questions or inquiries, you can reach out to the project creator on Instagram: [@thomas_is_lame](https://www.instagram.com/thomas_is_lame/)
Tag me if you use it! 

## Prerequisites
Before you start, ensure you have the following:

- A Raspberry Pi set up as per the instructions provided in the [rpi-rgb-led-matrix repository](https://github.com/hzeller/rpi-rgb-led-matrix).
- Basic knowledge of working with submodules in Git.

## Getting Started
Clone this repository and initialize the submodule by running the following commands:

```
git clone https://github.com/USERNAME/ohsillyscope.git
cd ohsillyscope
git submodule update --init
```

Navigate to the matrix directory and build the matrix submodule:

```
cd ./matrix
make
```

Return to the root directory and build the OhSillyScope project:

```
cd ..
make
```

## Configuration 
Please note that the setup process might require additional libraries, particularly for the matrix submodule. If you encounter issues during this step, refer to the [rpi-rgb-led-matrix documentation](https://github.com/hzeller/rpi-rgb-led-matrix) for troubleshooting guidance.

## Running OhSillyScope
OhSillyScope utilizes ALSA to interpret audio signals. To run the project, you need to specify the audio interface you want to capture from. First, identify your audio interface by running:

```
aplay -l
```

Identify the card number corresponding to your desired audio input. For instance, if your interface shows up as card 1, the startup command would be:

```
./osc "hw:1"
```

## Contributing
If you find ways to improve this project or fix any issues, please submit a pull request. Your collaboration helps make this project better for everyone.
