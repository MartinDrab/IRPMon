<h1>IRPMon</h1>
<p>
The goal of the tool is to monitor requests received by selected device objects or kernel drivers. The tool is quite similar to IrpTracker but has several enhancements. It supports 64-bit versions of Windows (no inline hooks are used, only moodifications to driver object structures are performed) and monitors IRP, FastIo, AddDevice, DriverUnload and StartIo requests.
</p>
<p>
The tool should compile fine in Microsoft Visual Studio 2013. MSVS 2015 seems to cause problems due to tons of warnings. I will have a look at the problem soon.
</p>
<h2>How to Test It</h2>
<p>
To test the program, you have to compile it and than use the irpmonconsole applicaton. 
</p>
<ol>
<li>Install the driver by irpmonconsole --install </li>
<li>Hook the driver you wish by irpmonconsole --hook-driver <DriverObjectName></li>
<li>Specify devices you wish to monitor by a sequence of these commands:
irpmonconsole --hook-device-name <DeviceObjectName>
irpmonconsole --hook-device-address <DeviceObjectAddress>
</li>
<li>Use irpmonconsole --monitor to log the requests (to the standard output).
</li>
</ol>
<p>
If you wish to monitor only new devices created by certain driver (the term "new" refers to a device created after the driver had been hooked) use irpmonconsole --hook-driver-nd <DriverObjectName> instead of the --hook-driver command.
</p>
<p>To get information about driver and device objects present in the system, you may either use the irpmonconsole --enumerate command, or take advantage of my VrtuleTree utility.
</p>
<p>To enumerate hooked objects and their IDs (handles), use the irpmonconsole --enumerate-hooks command. You can use the handles returned to unhook certain drivers or devices (--unhook-driver, --unhook
-device).
</p>
