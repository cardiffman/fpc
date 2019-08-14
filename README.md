# fpc
TIM lazy interpreter

This interpreter uses Three-Instruction-Machine instructions to interpret simple functional programs. In addition
to the three instructions, there are arithmetic operations and a conditional form that uses a slight variant on one
of the instructions.

The point of this is more just to guide the development of the fpa project than to be used by itself.

Under way: Adding a let form. We'll follow the method of Peyton-Jones and Lester which uses a special Move instruction
to initialize a closure with the value of a let-bound variable.

