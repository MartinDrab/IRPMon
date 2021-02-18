# IRPMon

The goal of the tool is to monitor requests received by selected device objects or kernel drivers. The tool is quite similar to IrpTracker but has several enhancements. It supports 64-bit versions of Windows (no inline hooks are used, only modifications to driver object structures are performed) and monitors IRP, FastIo, AddDevice, DriverUnload and StartIo requests.

## Compilation

### Drivers, Servers and DLLs

You need Visual Studio 2017 or 2019 to compile drivers and DLLs. All is set for the 2019, however, adopting the settings to 2017 one should be a piece of cake. In order to build the XP configuration (WIndows XP support), you need to install the Windows XP support into your Visual Studio. Also, install latest version of the Windows SDK and Windows Driver Kit (latest at the time of this writing at least). To use the `scripts\build.bat`, install MSBuild as well.

### GUI Application

Delphi 10.3 Rio does the trick here, although you should be able to compile the project also with Delphi XE2 and later. If you are looking for WIndows XP support, Lazarus seems to be your only option. Version 2.0.8 seems to produce pretty good results.

### Installer

The installer script (`scripts\installer.iss`) works with Inno Setup 6. That means, the resulting installer does not support Windows XP. Currently, there is no installer support for XP, just copy IRPMon files to machine with that old operating system and all should work quite nicely.

### Build Steps

* navigate to the `scripts` directory,
* build binaries with `build <Configuration>` where `Configuration` may be `Debug`, `Release` or `XP`,
* sign the binaries with `sign <Configuration>`, the `Configuration` parameter must match the previous step,
* build the installer with Inno Setup 6,
* sign the installer with the `sign-installer` script (no extra parameters are required).

You need to alter the signing scripts to respect your signing certificate. Similarly, the SDK version may need to be modified to match version installed on your machine.

## Samples

I hope to add some sample programs demonstrating how to use IRPMon DLLs to your advantage. So, you will not be dependent on the GUI application.

### Kbdsample

This sample hooks primary keyboard device (`\Device\KeyboardClass0`) and display basic information about detected requests. It also shows how to enumerate hooked drivers and unhook them inf necessary (e.g. if you wish to hook a driver that is already hooked, you may need to unhook it first). See the [kbdsample](kbdsample) directory for more information.

## People

### Authors

* [Martin Dráb](https://github.com/MartinDrab "Martin Dráb")

### Contributors

* [Petr Vaněk](https://github.com/arkamar "Petr Vaněk")

## Donations

If you wish to support development of this tool, you may donate some stuff to the following addresses:

* **BTC**: `17tEADhePvhHPj2X5GWn8vfiYhZCRH1f7V`
* **XMR**: `843axNixjKbLhibVSfx1NBVqhrjwfjw95EpbTZDL7uZuGyhQd4gj6TkFTkgRA1dhisPXURPTMMaY2QrT2fXDRAiF31E2sXP`
* [![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=6QWP4WH49Y3Z2&item_name=IRPMon&currency_code=CZK&source=url)

## Contact Me

Every feedback is more than welcome!

* you can create an issue on here on GitHub,
* or send an e-mail to **martin.drab@email.cz**.
