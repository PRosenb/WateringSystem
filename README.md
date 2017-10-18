# Arduino Watering System
https://github.com/PRosenb/WateringSystem

Arduino based watering system for up to three sprinklers connected to your water tap.

## Manual ##
See step by step description [Automatic Watering System](https://create.arduino.cc/projecthub/PRosenb/automatic-watering-system-160d90) at Arduino Project Hub.

## Features ##
- Automated watering
- Connected to your garden tap
- Up to 3 different watering zones
- Two valves in a row to minimize damage caused by valve failure
- Configurable watering time and duration
- Does not need many electrical parts

## Installation ##
This project requires a number of libraries to be installed first.

Required libraries:
- [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt)
- [DeepSleepScheduler](https://github.com/PRosenb/DeepSleepScheduler)
- [EEPROMWearLevel](https://github.com/PRosenb/EEPROMWearLevel)
- [DS3232RTC](https://github.com/JChristensen/DS3232RTC) (see its instructions on how to install it)

### Library Installation ###
The recommended way to install them is as follows (works for all except DS3232RTC).
- Open Arduino Software (IDE)
- Menu Sketch->Include Library->Manage Libraries...
- On top right in "Filter your search..." type the library's name
- Click on it and then click "Install"
- For more details see manual [Installing Additional Arduino Libraries](https://www.arduino.cc/en/Guide/Libraries#toc3)

### Watering System Installation ###
- [Download the latest version](https://github.com/PRosenb/WateringSystem/releases/latest)
- Uncompress the downloaded file
- This will result in a folder containing all the files for the library. The folder name includes the version: **WateringSystem-x.y.z**
  - Rename the folder to **WateringSystem**
  - Copy the renamed folder to your **Arduino** folder
  - From time to time, check on https://github.com/PRosenb/WateringSystem if updates become available

## Contributions ##
Enhancements and improvements are welcome.

## License ##
```
Arduino Watering System
Copyright (c) 2017 Peter Rosenberg (https://github.com/PRosenb).

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
