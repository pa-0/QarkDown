QarkDown
========

This is a GUI editor for [Markdown] files with [syntax highlighting][pmh] and the ability to execute Markdown compilers.

The following compilers are bundled with QarkDown:

- On OS X:
    - [Multimarkdown] by Fletcher T. Penney
    - [Discount] by David Parsons
- On Windows:
    - [Multimarkdown] by Fletcher T. Penney
- On Linux:
    - [Multimarkdown] by Fletcher T. Penney

Copyright © Ali Rantakari
<http://hasseg.org/qarkdown>

[Markdown]: http://daringfireball.net/projects/markdown
[Multimarkdown]: http://fletcherpenney.net/multimarkdown/
[Discount]: http://www.pell.portland.or.us/~orc/Code/discount/
[pmh]: http://hasseg.org/peg-markdown-highlight


Build Requirements
==================

Qt 6.8.0


Installing
==========

## Windows

Just run the installer.

## OS X

Just drop `QarkDown.app` into the `Applications` folder (or wherever you like).

## Linux

As of this writing no ready-made packages exist for Linux package managers, so you'll have to build and install QarkDown manually:

- Install the Qt SDK (from the Qt web site or via your package manager if it's available there)
- Open the `src/qarkdown.pro` file in Qt Creator
- Select the "release" configuration and press the build button (both of these are in the toolbar on the left side of the screen, at the bottom)
- Copy the executable (`qarkdown`) wherever you like.



Uninstalling
============

## Windows

**Note:** The uninstaller does not touch QarkDown's "application resources" folder, so if you want to remove it you'll have to do it manually. Its location depends on the version of Windows you're running, so if you don't know where it is, you can find it by clicking on the _"Show HTML Template File"_ link in the _"Compiling"_ section of QarkDown's preferences. Which you should of course do _before_ you run the uninstaller.

To remove the application itself, just run the uninstaller. You can run it from under the start menu, or by executing it directly from the QarkDown installation folder under `Program Files`.

## OS X

To remove the application, just delete `QarkDown.app`.

If you want to remove the application resources, you can find them in `~/Library/Application Support/QarkDown`.

## Linux

**Note:** If you want to remove the application resources folder but don't know where it is, you can find it by clicking on the _"Show HTML Template File"_ link in the _"Compiling"_ section of QarkDown's preferences. Which you should of course do _before_ you remove the application itself.

To remove the application, just delete QarkDown's executable from where you moved it at installation time.



