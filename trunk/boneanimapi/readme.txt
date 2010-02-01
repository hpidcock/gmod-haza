Simply include("sh_animationloader.lua") serverside and client side.

Add AnimLoader_LoadAnimFile("animation.txt") for each animation you want loaded.

The format of the files is a glon encoded text file with the following table structure:

{AnimationName = "Registered name", AnimationData = {FrameData, Interpolation, TimeToArrive, Type, RestartFrame, StartFrame}}


The included example animation's name is anim_lower_pistol