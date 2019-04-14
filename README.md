<h1>IRPMon</h1>
<p>
The goal of the tool is to monitor requests received by selected device objects or kernel drivers. The tool is quite similar to IrpTracker but has several enhancements. It supports 64-bit versions of Windows (no inline hooks are used, only modifications to driver object structures are performed) and monitors IRP, FastIo, AddDevice, DriverUnload and StartIo requests.
</p>
<p>
The tool should compile fine in Microsoft Visual Studio 2013. MSVS 2015 seems to cause problems due to tons of warnings. I will have a look at the problem soon.
</p>
<h2>How to Test It</h2>
<p>
To test the program, you may compile it and than use the <strong>irpmonconsole</strong> application. If you cannot or do not want to compile the project yourself, use the binaries in the <code>binaries</code>
</p>
<ol>
<li>Install the driver by <code>irpmonconsole --install</code> </li>
<li>Hook the driver you wish by <code>irpmonconsole --hook-driver &lt;DriverObjectName&gt</code></li>
<li>Specify devices you wish to monitor by a sequence of these commands:<br/>
<code>irpmonconsole --hook-device-name &lt;DeviceObjectName&gt;</code><br/>
<code>irpmonconsole --hook-device-address &lt;DeviceObjectAddress&gt;</code>
</li>
<li>Use <code>irpmonconsole --monitor</code> to log the requests (to the standard output).
</li>
</ol>
<p>
If you wish to monitor only new devices created by a certain driver (the term "new" refers to devices created after the driver had been hooked) use <code>irpmonconsole --hook-driver-nd &lt;DriverObjectName&gt;</code> instead of the <code>--hook-driver</code> command.
</p>
<p>To get information about driver and device objects present in the system, you may either use the <code>irpmonconsole --enumerate</code> command, or take advantage of my <a href='https://github.com/MartinDrab/VrtuleTree' title='VrtuleTree'>VrtuleTree</a> utility.
</p>
<p>To enumerate hooked objects and their IDs (handles), use the <code>irpmonconsole --enumerate-hooks</code> command. You can use the handles returned to unhook certain drivers or devices (<code>--unhook-driver</code>, <code>--unhook
-device</code>).
</p>
<p>
Also, a new version with some improvements, especially related to filtering new devices and devices belonging to certain device setup classes, is coming soon (I hope at least).
</p>
<h2>Donations</h2>
<p>
If you wish to support development of this tool, you may donate some stuff (BTC) to the following address:
</p>
17tEADhePvhHPj2X5GWn8vfiYhZCRH1f7V
