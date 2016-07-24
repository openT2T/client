# OpenT2T Client
A multi-platform library for hosting OpenT2T in client applications

## Overview
The OpenT2T client library is intended to make it easy for applications to host OpenT2T functionality, including a NodeJS (or compatible) engine that executes cross-platform OpenT2T JS APIs and translators.

Currently only the node hosting APIs are implemented, but eventually the
[OpenT2T JS library](https://github.com/opent2t/opent2t/) will also be made conveniently available to applications.

## INodeEngine interface
The core of the NodeJS hosting API is the cross-platform C++ [INodeEngine](./node/src/common/INodeEngine.h)
interface. Below this interface one or more node engine implementations may exist, possibly depending
on the platform. Above this interface are application-level APIs for using that interface directly and
for accessing OpenT2T JS APIs through it.

You will need to grab jx.lib as this project has a dependency on it. To do that, run the following powershell script on the external directory included in this repo: openT2T\client\node\src\external\jxcore\DownloadJxcoreLib.ps1

## Multi-platform and cross-platform APIs
There are library projects for [Android](./node/src/android), [iOS](./node/src/ios), and
[Windows](./node/src/windows) that expose the NodeJS engine and OpenT2T APIs in the corresponding
application-programming environments: Android Java, iOS Obj-C, and Windows WinRT. Additionally,
those APIs get automatically wrapped as plugins for Cordova and Xamarin cross-platform frameworks.


## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
