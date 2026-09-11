# User Manual

This guide explains how to start FIR, configure the station, use the main interface features, and shut the system down correctly.

## Starting the system and initial configuration

Download and extract the package that matches your hardware. Double-click `FIR.exe`. The web interface opens automatically at `http://localhost`.

On the first run, enter the operator callsign in the **Station configuration** section and click **Save**. FIR reports whether the operation succeeded.

![Initial configuration before saving](images/pre_guardar.png)

![Initial configuration saved successfully](images/post_guardar.png)

*Station configuration process. Source: original work.*

FIR starts in standby mode. Click the green **Resume engine** button in the upper-right corner to start audio capture and processing. The system transcribes audio received from the input device selected in Windows.

![Interface in standby after startup](images/pre_reanudar.png)

![Audio monitoring active](images/post_reaunadr.png)

*Audio engine initialization and operating states. Source: original work.*

## Transmitting audio from text

Type the message into the transmission module text box and click **Send**, or press `Enter`. FIR converts the text to audio using the configured TTS engine.

## Contact management and validation

When FIR detects text matching a radio callsign, it queries QRZ.com if credentials have been configured and retrieves the associated data.

If the callsign is not detected automatically, enter it in **Manual contact entry**. The interface displays the validation result.

![Callsign pending validation](images/pre_consulta.png)

![Retrieved data displayed in the interface](images/post_consulta.png)

*Contact data enrichment process. Source: original work.*

After the data has been verified, click the validation button to record the contact in the local `logbook`. Clicking a contact log entry opens the matching QRZ.com profile in the browser.

## ADIF export and shutdown

FIR can generate a report in the international ADIF standard (*Amateur Data Interchange Format*). Click the export button to generate a downloadable file compatible with third-party software.

![Export panel](images/pre_export.png)

![Generated report file](images/post_export.png)

*Data and report output management. Source: original work.*

To close FIR, use **Stop entire system** in the **System management** section. This control stops the audio engine and web server cleanly.

![System running before shutdown](images/pre_apagar.png)

![Final state after processes stop](images/post_apagar.png)

*Controlled shutdown sequence. Source: original work.*
