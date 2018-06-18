# gbend~ for Max

## Requirements :

You need the [Max SDK](https://cycling74.com/downloads/sdk/) to build this external.

## Compilation :

This repository should be placed inside a folder located at the root of the SDK, next to the `c74support` folder.  
Open the project file corresponding to your platform in `build/<your_platform>/` and build the project.
The binary file will be built into `bin/<your_platform>/`. Binary is already included in `bin/osx/` for convenience.

## Installation :

Move the external file from `bin/<platform>/` and the help patcher file from `bin/help/` to any folder located in  
`Documents/Max 7/Library`.

## Information about the object :

`[gbend~]` is a monophonic (multitrack but non polyphonic) audio sample player object for Max, working with the `[buffer~]` object,
which allows to play grains without having to worry about audio clics.  
  
Its special features are :
- It integrates fade-in and fade-out linear gain ramp parameters, expressed in relative milliseconds (relative to the grain's current playing speed),
as well as an interrupt ramp parameter (quick fade-out before rewind), expressed in absolute milliseconds.  
  
- It has one signal input, which is evaluated as a pitch parameter (playing speed), expressed in relative semitones :
for example a signal with a constant value of 12 will play the grain twice faster than its original speed,
that is to say one octave above its original root note. A value of 7 will play the grain one fifth above its original root note,
plugging the output of a `[cycle~]` object will create some frequency modulation effects, etc.  
  
For more info, see the help patcher.
