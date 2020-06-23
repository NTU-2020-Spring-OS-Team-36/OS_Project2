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
sudo sh test.sh <test_files_directory> <method> <ip_address>
```

e.g. `sudo sh test.sh sample_input/sample_input_1 mmap 127.0.0.1`

The test script will iterate through all the files in the specified directory and send all of them as the input file args of master program.

For further info, please checkout `test.sh`. Feel free to modify it.

### VM (Optional)

[Here](https://drive.google.com/file/d/134GFXJmH6iV647lIGna9k4dhd-IV7OFj/view?usp=sharing) is the .ova file of ubuntu 16, log in account of CSIE Gsuit to download it. Remember to select kernel version __4.14.25__ in __Advance options for Ubuntu__ at grub menu.