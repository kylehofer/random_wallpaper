# Random Wallpaper
## Dependencies
Requires opencv and gtkmm

## Description
A simple application for creating random wallpaper collages for use with Ubuntu. Tested and ran on Ubuntu 24.10.
Uses xrandr to calculate the number, positions and resolutions of screens, opencv to crop and zoom images to fit screens it will be displayed on, and gtkmm for changing gsettings.

## Usage
random_wallpaper /path/to/wallpaper/folder [time]

### Example
Creates a wallpaper from ~/Pictures/wallpapers every 60 (default) seconds
```random_wallpaper ~/Pictures/wallpapers```
Creates a wallpaper from ~/Pictures/wallpapers every 15 seconds
```random_wallpaper ~/Pictures/wallpapers 15```

### Service Example
As this application requires access to the displays, it cannot be ran as root.
You can setup the service using user specific systemd services by configuring it in `~/.config/systemd/user/`, and then `systemctl` with the `--user` flag