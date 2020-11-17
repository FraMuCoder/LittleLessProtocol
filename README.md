# Little Less Protocol

Little Less Protocol is a simple serial protocol for little microcontrollers.
In comparison to other protocols Little Less Protocol uses less buffers and less states.
Also you application not need full buffers but if you want to use them,
they are owned by the application, so your application has the full control over it.

I will start with Little Less Protocol A, an ASCII version. All non ASCII values are
transmitted as hex strings so you can use every ASCII based serial terminal
for first tests.
This protocol can be used while developing your project.

If there is a A there should be also a B of course.
Little Less Protocol B will be the binary version of this protocol. After finishing
the serial communication part of your project you should easily switch to the B version.

## Install

Just download the code as zip file. In GitHub click on the `[Code]`-button and select `Download ZIP`.

In Arduino IDE select `Sketch` -> `Include library` -> `Add ZIP Library ...` to add the downloaded ZIP file.

## Links

* [Repository](https://github.com/FraMuCoder/LittleLessProtocol)

## License

Little Less Protocol is distributed under the [MIT License](./LICENSE).
