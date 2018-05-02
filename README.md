# Digital Orrery

This is a project to create a digital [orrery][orrery], each planet can be addressed and controlled individually.

## Parts list

* [4x Nema 17 stepper motors, 40mm][motors]
* An Arduino Uno
* [Arduino CNC shield][cnc]
* All of the 3D printed parts

## Current Functions

Currently the orrery has two functions, animate and date mode. In its default mode, the orrery slowly spins the four planets at realistic relative speeds meaning that Mars spins 8 times slower than Mercury. The accuracy of the animation is acceptable as it is within 4% of real speeds, although the method used creates unrealistic cycles that happen noticeably often.  
The second function is to snap to a position when given a date over serial. When given an input date, the number of days between the given date and 1/1/2000 is calculated. The actual positions of the planets at 1/1/2000 is known as well as how far the planets move in a day, so it is as simple as multiplying the day delta by the planet speed to find its new location. 

## Problems  

There are a number of problems with this device, namely that it slowly drifts out of alignment and becomes inaccurate. The source of this inaccuracy is from stepper motors missing steps, so a solution may be to increase power to the steppers or to measure where the planets are via a rotary encoder.   
In a similar vein, when the orrery is started it takes its current state as being zeroed out and will assume all the planets are aligned. This is undesired because it means that every single time it is started it needs to be realigned. This problem would also be fixed with rotary encoders. 

## Future Plans

Things I would like to add or fix:  
* Some way for the arduino to know where the planets are, either a rotary encoder or a bump that a switch can detect. 
* An LCD screen and buttons to allow user input without the need for a computer.
* An enclosure for the arduino and shield to make transportation easier, as well as improve aesthetics. 

## Demonstration

Whole device:  
![Whole device][wide]  

Close up:  
![Closer device][close]

Video:  
[![Video](http://img.youtube.com/vi/tbyMe05wM1g/0.jpg)](http://www.youtube.com/watch?v=tbyMe05wM1g "Orrery Working")

More info found [here][thingiverse]

[thingiverse]: https://www.thingiverse.com/thing:2889266
[motors]: https://www.amazon.com/gp/product/B00PNEQI7W
[cnc]: https://www.amazon.com/gp/product/B06XJKVLG3
[wide]: https://i.imgur.com/hOvwWhX.jpg 
[close]: https://i.imgur.com/2LTnKSo.jpg
[orrery]: https://en.wikipedia.org/wiki/Orrery
