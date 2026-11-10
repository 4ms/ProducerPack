# Changelog 

## v2.0.5

- TapeDelay: reduced CPU load on MetaModule, and removed an audio dropout when the module is first loaded
- Fix labels on TapeDelay Time and Filter knobs on MetaModule (Filter now is -100% to 100%)
- TapeDelay: new input no longer cuts off the repeats. The input is recorded clean, and the filter, saturation and tape coloration happen only in the feedback loop
- TapeDelay: Width no longer clicks or chops the repeats. It now uses two cross-fed delay lines (true ping-pong) instead of an alternating panner

## v2.0.4

- Fix label on DJFilter freq knob on MetaModule (now is -100% to 100%)

## v2.0.3

- Add TapeDelay module
- Fix small memory leak in BitCrusher and DJFilter

## v2.0.2

- Add Bypass to most modules

- Add Polyphony support for most modules


## v2.0.1

- DJ Filter: repaired highpass side of filter. Previously, the filter was behaving as a bandpass. 

## v2.0.0

- Initial release 

