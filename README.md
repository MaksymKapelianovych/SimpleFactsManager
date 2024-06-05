Simple plugin for managing facts which can track various data: narrative facts (is player talked to NPC2? did player do optional action in quest?), achievements (just count how many actions were done and check if it is enough for achievement).


Example: we have level with 3 trigger boxes: Quest, Interact and Dialogue. 
Dialogue can be played only after Quest started (player entered Quest trigger) and it can be played only once. 
Interact becomes active only after dialogue is finished and it can be triggered unlimited amount of times.

![image](https://github.com/MaksymKapelianovych/SimpleFactsManager/assets/48297221/4dc0423e-b9d1-41a0-b622-87a0c8e72abe)

![image](https://github.com/MaksymKapelianovych/SimpleFactsManager/assets/48297221/1dfa2fd9-3b07-4423-b65d-26e66d082cd2)


All nodes the plugin have:
 - ChangeFactValue: allows to set fact's value or add value to fact's current value. If fact was undefined before this operation, it will become defined.
 - ResetFactValue: reset fact's value to default (0). Only defined facts can be reset now.
 - TryGetFactValue: returns fact's value if fact is defined. Also returns boolean, indicating if fact is defined.
 - CheckFactValue: compares fact's value with WantedValue based on passed EFactCompareOperator.
 - CheckFactSimpleCondition: the same as CheckFactValue, but takes FSimpleFactCondition as parameter (it allows to store and reuse conditions).
 - IsFactDefined: specialized version of CheckFactValue, which only tells if fact is defined or not.
   
![image](https://github.com/MaksymKapelianovych/SimpleFactsManager/assets/48297221/892ebc9e-5038-4a9b-893b-043eb4eb9d27)


Fact is just a FFactTag, stored inside TMap in UFactSubsystem. If TMap does not contain some FFactTag, it means that such fact is not defined yet, but can be defined later. All subtags of "Fact" tag are counted as valid possible facts.
Value of Fact is int32. This should be enough for covering basic needs. For example, it can be used as boolean (0 is false, 1 is true), integer (obviously), float (range [0,1] with 2 digits precision can be represented as int range [0,100]).

Plugin also have simple SaveGame support (only facts in TMap are stored).


Console commands:
 - Facts.ChangeValue. Usage: Facts.ChangeValue Fact.Tag IntValue ChangeType [Default = Set]. Changes value of provided fact.
 - Facts.GetValue. Usage: Facts.GetValue Fact.Tag. Prints value of a fact to log.
 - Facts.Dump. Prints values of all defined facts
