# MountTracker

`mount-tracker` is a simple utility that allows you to track mount changes in real-time. 

## Dependencies

- `glib`
- `gcc`

## Building

To build `mount-tracker`, run the following command:

```bash
$CC $(pkg-config --cflags --libs glib-2.0) mount-tracker.c -o mount-tracker
```

## Usage

```bash
mount-tracker -d <device> -t <timeout>
```

- `-d`: A mandatory option that specifies the list of devices to monitor. Multiple devices can be passed as arguments.
- `-t`: An optional parameter that configures the timeout in milliseconds used by the `poll` system call.

## Example

```bash
mount-tracker -d sda -d sdb
```

**Note:** Make sure that the devices passed in `-d` are already mounted before running the application.

## License

`mount-tracker` is licensed under the [MIT License](https://opensource.org/licenses/MIT). 

## Contact

For questions, suggestions, or other inquiries, please contact Deva T at <Deva_Thiyagarajan2@comcast.com>.
