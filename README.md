Formation Flying Quadcopters
============================


This project is to take very cheap RC quadcopters (Cheerson CX10 used) and use a computer to remote control them with greater accuracy than a human could to allow formation flying and collaborative task completion.



Architecture
------------


![System design diagram](docs/quad.png)


Project Status
--------------

All experimental.   Nothing works fully yet.


Build log
---------

1. Messages successfully sent to/from quadcopter using bus-pirate -> SPI -> XN297 -> quadcopter.  Determined on-air timing was critical to message acceptance.
2. Initial convnet working to find basic quadcopter moving objects.  Lots of tweaking still possible for better performance.
3. Added training data video with the first ~300 frames manually annotated.

