# maya-usd-ufe-sketch

This is a quick sketch outlining some ideas regarding the general structure of the maya-usd/UFE integration with studio plugins. The basic premise is that the studio _(in this case Animal)_ provides an interface which the UFE/USD integration uses in order to determine the editability and status of the xform ops in the stack.

The main API is designed around a IPrimBusinessLogicRegistrar class, which maintains a chain of IPrimBusinessLogic objects. In this super MVP example, the business logic object has to:

* Provide on demand, a complete meta-data description of what can, and cannot be done to a given attribute _(i.e. default value v.s. currentFrame, read only v.s. mutable, etc)_
* As a layer on top of the raw attribute layer, be able to provide similar handling explicitly for xform ops _(we may need additional infomation, such as coordinate frames, etc. Most of this should be able to be computed)_
* When the translate, scale, or rotate tools are activated on a specific prim, the business logic should provide the xform ops that those tools should operate on by default _(allows studios to use any conventions they want)_
* Creating those TRS xform ops _(or not, the choice is yours!)_ in the correct place within the ordered xform ops.

 The main reason is that I want to set up a chain of objects which determine the business logic, is that rather than juggling a matrix of options and flags _(e.g. push2Prim/readAnimatedValues)_, an object in the chain can intercept a request for meta-data about an xform op, or it can pass it down the chain if it doesn't care about that prim. I imagine a very simple stack of business logic could look like:

```                 LightingDeptLogic --> ALDefaultLogic --> maya-usd-sensible-defaults```

The idea is that we ship maya-usd with just the 'sensible defaults' as the only logic object within the IPrimBusinessLogicRegistrar, and later studios/departments can push their own logic objects into the registrar. 

## Quick Overview of the code

Within the api folder is the main API that would reside within the core maya-usd lib. The main file of interest is IPrimBusinessLogic.h, which tries to define a minimumal interface with which a studio can provide maya-usd with the business logic it needs. 

maya-usd probably wouldn't interact with that interface directly, but instead interact with the internal class you see in maya-usd-side/PrimBusinessLogic.h. The idea is to create the smallest/simplest interface possible for the maya-usd devs, one that allows for all of these descisions to be kicked down the road for specific studios/departments to worry about.

Finally, on the animal logic side of things _(a.k.a. other studios)_, we'd provide one or more logic objects so that maya-usd no longer has to worry about whether something should be read only, or what timecode you should be using. The studio will just tell maya-usd what to do :) 

## Other thoughts

* I assume we might want to provide business logic objects written in python? 
* I'm a bit concerned that all these virtual calls might hurt performance when selecting huge number of objects
* This push/pop logic paradigm could also extend itself to in house tools. e.g. EnvironmentStudioLogic. 
* Is there a better way to achieve the same result?

## Things I haven't included

* Computing the coordinate frame for the manipulator. It's assumed you can simply iterate over the xform ops, accumulating a matrix as you go. 
* Quality code - this is just a rough sketch!

## Possible things to think about....

* I am assuming that maya-usd would have a fallback mechanism to work based on a database of prim types, and their attributes _(and whether they should be keyed or not, etc)_. This would be nice if the config for this was stored as USD data _(because then overrides could be inserted at the studio/dept/user/shot level, and stage composition should take care of setting sensible defaults)_.  