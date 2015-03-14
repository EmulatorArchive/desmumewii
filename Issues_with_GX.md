Now that Desmume can run on native GX (translation: hardware acceleration baby!), let's look at some of the pitfalls and problems that plague us.

# What's wrong with GX? #

There is actually very little wrong with GX, it is actually Desmume, and how it does its work, that is causing the trouble when we move to native GX code.

Problems:
**Sprites**Textures

I will be using Super Mario 64 DS as an example. If you load up the game, you will see something very, very odd: What looks like four blue buttons on the top screen and blue gibberish on the bottom screen. What? It's broken! Not so. Here is why it does this:

I direct your gaze to the Draw() function in main.cpp. When we use the software renderer, everything ends up coming out in the wrong format, so we need to convert it to be 4x4 texels (what the Wii likes) in order to render it to the screen. Notice the line:

> `if(current3Dcore == 1)`

If current3Dcore is one, then we want to use GX, (if it is two, we use the software renderer). Notice that within that if statement we are not "converting" the image before we render it to the screen. If you comment this out, you will see that the text is fine, but woe! The 3D has become gibberish! This is because GX will, obviously, render 3D in the correct format! But, the sprites (text) are not in the correct format, since these are generated via the Desmume software.

This also leads into the discussion of textures. You may still call the 3D star at the start of SM64DS a failure since it is clearly the wrong color, but in actuality I believe this is due to the same problem that we have rendering sprites: incorrect format. I am not 100% certain that this is the case, however. I could be wrong.

## Possible Solutions ##

So what's the solution? Thankfully, I think I have managed to track down what is needed to be done, and I have a few possible alternatives.

#1. Convert the Sprites, but not the 3D.

This would involve finding the source of the sprite generation. What we could do is apply the "texel conversion" normally done in the Draw() function to the sprite upon its creation.

#2. Render in layers.

Desmume uses a layered structure for its rendering engine. It will loop through every line and apply the background, sprites, 3D, and more sprites on top of that.

  * The start of line 2087 does the background layer
  * Line 2133 renders sprites with the function: "gpu->spriteRender"
  * 3D is rendered on line 2180: "gpu->setFinalColor3d"
  * And the final sprites are done on line 2212: "gpu->setFinalColorSpr"

There's some more in there but this is a good example. The software will compare alpha combine pixels for each layer in the line. This is (I believe) accomplished in the function: GPU\_RenderLine\_layer in GPU.cpp.

It is possible to render each layer as a separate entity, convert the sprite layers, and combine them  with the rendered 3D image (it's a texture) as a multi-layered texture using the GX hardware.

#3. "Unconvert" the 3D.

I do not like this one, but I thought that I should suggest it nonetheless. This would involve doing the exact opposite of the 4x4 texel conversion to the rendered 3D image. That way, when we convert it later, it will look correct. This, obviously, would involve more rather than less CPU cycles; the entire reason to use GX in the first place. This could be a quick-fix until a more permanent solution is found.

## Problems and Roadblocks ##

#1. The GPU renders things in lines. I do not know where the sprites are stored. Speaking with the "Vanilla" Desmume developers, it seems that they are taken directly from the DS game. What does this mean? It means (unless there IS a big storage place that houses all the sprite layers, which would be awesome) that it may be very difficult to convert the individual lines into 4x4 texels, since each line would not comprise all the information that a 4x4 texel would need. Again, I am not 100% sure about this, so maybe there is hope.

#2. This may involve significant rewriting of the Desmume GPU code in order to get native GX to run at full efficiency. This may affect the software renderer. This is just a possibility, however. Someone more familiar with the software renderer should be able to address this better than I.

## Conclusion ##

I would vote for the "Render in Layers" approach. I believe the tradeoffs (having to hold space for each layer) would be offset by the fact that the hardware could combine them at a FAR greater efficiency than the software ever could. Look in the function "