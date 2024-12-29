# Simple Facts Manager
Simple plugin for managing facts which can track various data: narrative facts (has player talked to NPC2? has player made an optional action in quest? how many times player failed some puzzle?), achievements (just count how many actions were done and check if it is enough for achievement).

## Example 
We have level with 3 trigger boxes: Quest, Interact and Dialogue. 
Dialogue can be played only after Quest started (player entered Quest trigger) and it can be played only once. 
Interact becomes active only after dialogue is finished and it can be triggered unlimited amount of times.

![image](https://github.com/MaksymKapelianovych/SimpleFactsManager/assets/48297221/4dc0423e-b9d1-41a0-b622-87a0c8e72abe)

![image](https://github.com/user-attachments/assets/cfcbaf65-6b07-441f-9582-cd7a2627c0b3)

## Overview
Fact is just a FFactTag, stored inside TMap in UFactSubsystem. If TMap does not contain some FFactTag, it means that such Fact is not defined yet, but can be defined later. All subtags of "Fact" tag are counted as valid possible Facts.
Value of Fact is int32. This should be enough for covering basic needs. For example, it can be used as boolean (0 is false, 1 is true), integer (obviously), float (range [0,1] with 2 digits precision can be represented as int range [0,100]).

To make it easier to load predefined Facts values for debug or to set up preconditions for gameplay logic, there is a separate asset: `UFactPreset`. To create it right click in Content Browser, navigate to `Miscellaneous > Data Asset` and select `Fact Preset`.
Presets can be loaded at runtime via `UFactStatics::LoadFactPreset(s)`, by right-clicking on presets and picking `Load preset(s)` option or by selecting presets in `Preset` picker in Fact Debugger window. 

### Nodes:
![image](https://github.com/user-attachments/assets/4263d2a6-eefc-4f0b-9954-6c926c18816f)

 - `ChangeFactValue`: allows to set Fact's value or add value to Fact's current value. If Fact was undefined before this operation, it will become defined.
 - `ResetFactValue`: resets Fact's value to default (0). Only defined Facts can be reset now.
 - `TryGetFactValue` (**deprecated**): returns fact's value if fact is defined. Also returns boolean, indicating if fact is defined.
 - `GetFactValueIfDefined`: returns Fact's value if Fact is defined. Also splits execution flow, depending on whether Fact is defined.
 - `CheckFactValue`: compares Fact's value with WantedValue based on passed EFactCompareOperator.
 - `CheckFactSimpleCondition`: the same as `CheckFactValue`, but takes ``FSimpleFactCondition`` as parameter (allows to store and reuse conditions).
 - `IsFactDefined`: specialized version of `CheckFactValue`, which only tells if Fact is defined or not.
 - `LoadFactPreset`: loads Facts values, defined in FactPreset.
 - `LoadFactPresets`: same as `LoadFactPreset`, but accepts TArray of `FactPreset`.
   
Plugin also have simple SaveGame support (only defined Facts in UFactSubsystem are stored).

## Debug
Plugin provides Fact Debugger window, which allows to monitor and change Fact values at runtime. Window can be opened in editor, `-game` mode or Debug/Development builds by navigating to `Tools > Debug > Fact Debugger` (only in editor) or by console command `Facts.Debugger`.

All Facts, that exist in project, are displayed in two trees (Favorites and Main). Facts can marked as Favorite by clicking on star icon.

![image](https://github.com/user-attachments/assets/412373df-9306-4fec-a580-b6e8fada453f)

1. Tree settings:
   1. Show Favorites in Main tree.
   2. Show only leaf Facts (**Note! If you have defined Fact with subtags, it will not be shown in trees when this option is checked!** .
   3. Show only defined Facts.
2. Picker for `FactPreset`.
3. Search bar. Facts can be searched by single word or by several words, separated with spaces ("quest trigger"), and Fact will be shown if it contains all search words.
Search strings can be saved as separate filter (plus button).
4. All settings for Fact Debugger window.
5. Saved search filters. If several filters are active, Fact will be shown if it passes any active filter.
6. Favorites tree. Displays only Facts, marked as favorite. If favorite Fact has children, all children will also be in this tree.
7. Main tree. Displays all Facts, except of favorite ones (but can show them, if setting "Show Favorites in Main tree" is checked).
8. Displayed and total count of Facts in Favorites tree.
9. Displayed and total count of  Facts in Main tree.

### Console commands:
 - `Facts.ChangeValue`. Usage: Facts.ChangeValue Fact.Tag IntValue ChangeType [Default = Set]. Changes value of provided Fact.
 - `Facts.GetValue`. Usage: Facts.GetValue Fact.Tag. Prints value of a Fact to log.
 - `Facts.Dump`. Prints values of all defined Facts.
 - `Facts.Debugger`. Brings up FactDebugger window.
