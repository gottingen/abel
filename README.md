# abel - c++ base library

abel is an open-source collection of c++ library code. it designed to use c++ smartly.

![abel](https://github.com/gottingen/abel/blob/master/docs/images/abel.png)

## status

[![Build Status](https://www.travis-ci.org/gottingen/abel.svg?branch=master)](https://travis-ci.org/gottingen/abel)

## content index

* [about abel](#about)
* [about cmake](#cmake)
* [build abel](#build)
* [modules](#modules)
* [examples](#examples)

<a name="about"> </a>

## about abel

For several years, I have planned to establish a repository to de-duplicate code from the projects either I have been 
involved in or I have paid close attention to (e.g.,open source) so that I would develop iteratively based upon the 
repository. I have tried the codes on many private projects and the applicability is verified. The  repository , named 
after a mathematician as 'Abel', fortunately came out on Jan 1st 2020.


Since the inception, it's aimed to <font size=16> **`consolidate algorithms, data structures, system operations and 
make sure it's under control.`**</font>
Particularly, the goals are:

* to have a library that has been well implemented and tested containing tools and algorithm.
* aim high modularity with reduced dependencies between modules.
* zero external dependencies.
* build on all platforms with c++, such as linux, mac, android, windows, mobiles.
* no warning and bugs on any platform and compiler.
* published interfaces are required to have full documentation， using case description，performance benchmark and evaluation.
* keep overhead down, compress overall size.


collect code and ideas from:

* [abseil-cpp](https://github.com/abseil/abseil-cpp) from google, and make abel basic framework
* [EASTL](https://github.com/electronicarts/EASTL) from EA
* fermat my private cpp lib


<a name="cmake"> </a>
 
## about cmake


<a name="build"> </a>

## build abel

abel use cmake as build system. sample to build abel

compiler requirement

    - clang version > 3.3
    - gcc version > 4.8
    - cmake version > 3.5(if you build benchmark) otherwise 2.8 is enough
build step

    $ git clone https://github.com/gottingen/abel.git
    $ cd abel
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ make test
    
<a name="modules"> </a>

## modules

<a name="examples"> </a>

* [base](/docs/en/base.md)
    
    the base module contains code that other modules depends on. no extern dependencies.
* [algorithm](/docs/en/algorithm.md)
* [atomic](/docs/en/atomic.md) 
* [container](/docs/en/container.md)
* [debugging](/docs/en/debugging.md)
* [digest](/docs/en/digest.md) 

    the digest module contains md5, sha1, sha256 tools.
* [flags](/docs/en/flags.md)
* [format](/docs/en/format.md)
* [strings](/docs/en/strings.md) 

    strings library contains string utilities, such as trim, split. also include a 
    compatible version of string_view.


## examples