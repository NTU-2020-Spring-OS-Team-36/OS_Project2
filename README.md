# OS Project 2

### System info

* kernel version: 4.14.25

### Compile

```
sh clear.sh
sh compile.sh
```

### Test

```
./master [-fma] [files]...
./slave [-fma] ip_address [files]...
```

The test script will iterate through all the files in the specified directory and send all of them as the input file of user programs. The received file of slaved would be saved in the `output/` directory.

For further info, please checkout `test.sh`. Feel free to modify it.

### VM (Optional)

[Here](https://drive.google.com/file/d/134GFXJmH6iV647lIGna9k4dhd-IV7OFj/view?usp=sharing) is the .ova file of ubuntu 16, log in account of CSIE Gsuit to download it. Remember to select kernel version __4.14.25__ in __Advance options for Ubuntu__ at grub menu.
