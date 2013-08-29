/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2013 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef __CompositorTextureDefinitionBase_H__
#define __CompositorTextureDefinitionBase_H__

#include "OgreHeaderPrefix.h"
#include "Compositor/OgreCompositorCommon.h"
#include "OgreIdString.h"

namespace Ogre
{
	/** \addtogroup Core
	*  @{
	*/
	/** \addtogroup Effects
	*  @{
	*/

	/** Centralized class for dealing with declarations of textures in Node &
		Workspace definitions. Note that shadow nodes use their own system
	@author
		Matias N. Goldberg
    @version
        1.0
    */
	class _OgreExport TextureDefinitionBase : public CompositorInstAlloc
	{
	public:
		enum TextureSource
		{
			TEXTURE_INPUT,		/// We got the texture through an input channel
			TEXTURE_LOCAL,		/// We own the texture
			TEXTURE_GLOBAL,		/// It's a global texture. Ask the manager for it.
			NUM_TEXTURES_SOURCES
		};

	protected:
		/// Local texture definition
        class TextureDefinition : public CompositorInstAlloc
        {
        public:
			enum BoolSetting
			{
				Undefined = 0,
				False,
				True,
			};

            IdString name;
            uint width;       // 0 means adapt to target width
            uint height;      // 0 means adapt to target height
			float widthFactor;  // multiple of target width to use (if width = 0)
			float heightFactor; // multiple of target height to use (if height = 0)
            PixelFormatList formatList; // more than one means MRT
			bool fsaa;			// FSAA enabled; True = Use main target's, False = disable
			BoolSetting hwGammaWrite;	// Do sRGB gamma correction on write (only 8-bit per channel formats) 
			uint16 depthBufferId;//Depth Buffer's pool ID.

			/** In D3D9, reading from an fsaa texture is not possible, hence it always has
				to be resolved before using it. When resolves are implicit, trying to read
				a render target as a texture will make Ogre to automatically resolve the
				FSAA rt into a normal one.
			@par
				For implicit resolves, try to render everything first, then use it as a texture
				instead of mixing reads & writes, to avoid excessive resolves on the same frame.
			@par
				In D3D10+ & GL 3.3+; FSAA surfaces can be read as textures (that is, without
				resolving) for advanced image manipulation (or even performing custom resolves).
				For this reason, turning Explicit resolves on will force Ogre not to resolve
				targets when used as a texture; resolving can still be done using a PASS_RESOLVE
				or using a PASS_QUAD with a custom resolve pixel shader.
			@par
				Explicit resolves are obviously not supported in D3D9, thus this flag is forced
				always to false, and PASS_RESOLVE passes are skipped.
			*/
			bool	fsaaExplicitResolve;

			TextureDefinition( IdString _name ) : name(_name), width(0), height(0), widthFactor(1.0f),
					heightFactor(1.0f), fsaa(true), hwGammaWrite(Undefined), depthBufferId(1),
					fsaaExplicitResolve(false) {}
        };

		typedef vector<TextureDefinition>::type		TextureDefinitionVec;
		typedef map<IdString, uint32>::type			NameToChannelMap;

		/** TextureSource to use by addLocalTextureDefinition. Could be either
			TEXTURE_LOCAL or TEXTURE_GLOBAL (depends on our derived class)
		*/
		TextureSource			mDefaultLocalTextureSource;
		TextureDefinitionVec	mLocalTextureDefs;

		/** Similar to @see CompositorNodeDef::mOutChannelMapping,
			associates a given name with the output.
		*/
		NameToChannelMap		mNameToChannelMap;

		static uint32 encodeTexSource( size_t index, TextureSource textureSource );
		static void decodeTexSource( uint32 encodedVal, size_t &outIdx, TextureSource &outTexSource );

	public:
		TextureDefinitionBase( TextureSource defaultSource );

		/// This has O(N) complexity! (not cached, we look in mNameToChannelMap)
		size_t getNumInputChannels(void) const;

		/** Adds a texture name, whether a real one or an alias, and where to grab it from.
		@remarks
			Throws if a texture with same name already exists, or if the name makes improper
			usage of the 'global' prefix.
		@par
			This is a generic way to add input channels, by calling:

				addTextureSourceName( "myRT", 0, TextureDefinitionBase::TEXTURE_INPUT );

			You're assigning an alias named "myRT" to channel Input #0
			For local or global textures, use addLocalTextureDefinition

		@param fullName
			The name of the texture. Names are usually valid only throughout this node.
			We need the name, not its hash because we need to validate the global_ prefix
			is used correctly.
		@param index
			Index in the container where the texture is located, eg. mLocalTextureDefs[index]
		@param textureSource
			Source where the index must be used (eg. TEXTURE_LOCAL means mLocalTextureDefs)
		@return
			IdString of the fullName paremeter, for convenience
		*/
		virtual IdString addTextureSourceName( const String &name, size_t index,
												TextureSource textureSource );

		/** Retrieves in which container to look for when looking to which texture is a given name
			associated with.
		@remarks
			Throws if name is not found.
		@param name
			The name of the texture. Names are usually valid only throughout this node.
		@param index [out]
			The index at the container in which the texture associated with the output channel
			is stored
		@param textureSource [out]
			Where to get this texture from
		*/
		void getTextureSource( IdString name, size_t &index, TextureSource &textureSource ) const;

		/** Reserves enough memory for all texture definitions
		@remarks
			Calling this function is not obligatory, but recommended
		@param numPasses
			The number of texture definitions expected to contain.
		*/
		void setLocalTextureDefinitions( size_t numTDs )		{ mLocalTextureDefs.reserve( numTDs ); }

		/** Creates a TextureDefinition with a given name, must be unique.
		@remarks
			WARNING: Calling this function may invalidate all previous returned pointers
			unless you've properly called setLocalTextureDefinitions
		@par
			@See addTextureSourceName remarks for what it can throw
		@par
			Textures are local when the derived class is a Node definition, and
			it's global when the derived class is a Workspace definition
		@param name
			The name of the texture. Names are usually valid only throughout this node.
			We need the name, not its hash because we need to validate the global_ prefix
			is used correctly.
		*/
		TextureDefinition* addTextureDefinition( const String &name );
	};

	/** @} */
	/** @} */
}

#include "OgreHeaderSuffix.h"

#endif