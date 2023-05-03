Pedometer

Authors : Vedang Gaikwad, Tony Luo

Hardware : Tiva TM4C123GH6PM with Orbit Booster Pack

This is a pedometer that tracks the amount of steps that a person has taken by attaching the hardware to the users hip and walking around. Upon turnign on the board, it will be in the "Steps taken - Raw" screen which keeps track of the amount of steps the user has taken. By hitting the up button, it will change the screen to the "Steps taken - % Goal" screen which tracks the percentage of steps taken towards the goal which the user can set manually. 

To do this, hitting the right button while in the "Steps" screen will take you to the "Setting Goal" screen. Here "The Goal" is the current goal of th user, and "Set Goal" is where you can change current goal. To do this, turning the potentiometer will increase the "Set Goal" value by in 100 increments, and capping off at 10,000 steps. To set a goal, click the down button once or hold it, and it will change "The Goal". Now the steps that the user takes will be a counted towards the new goal.

By hitting the right button again while in the "Setting Goal" screen will take you to the "Total Distance" screen. This screen tracks the amount of distance the user has travelled. This is measured in Km and Miles. Pressing the up button can switch between the two distances. Then hitting the right button again will take the user back to the "Steps taken" screen. 

The user is able to switch between the screen in the opposite direction as well by hitting the left button instead.

While in the "Steps taken" and "Total Distance" screens, holding the down button will reset the values to 0. 

After the user has reached the goal, a "Goal Reached!" message will appear. Pressing the right button will continue from here and the step and distance values will be set to 0 again.
