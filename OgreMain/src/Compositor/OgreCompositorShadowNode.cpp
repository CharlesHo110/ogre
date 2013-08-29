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

#include "OgreStableHeaders.h"

#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/OgreCompositorChannel.h"

#include "OgreTextureManager.h"
#include "OgreHardwarePixelBuffer.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
	CompositorShadowNode::CompositorShadowNode( IdType id, const CompositorShadowNodeDef *definition,
												const CompositorWorkspace *workspace,
												RenderSystem *renderSys ) :
			CompositorNode( id, definition->getName(), definition, workspace, renderSys ),
			mDefinition( definition )
	{
		//Create the local textures
		CompositorShadowNodeDef::ShadowMapTexDefVec::const_iterator itor =
															definition->mShadowMapTexDefinitions.begin();
		CompositorShadowNodeDef::ShadowMapTexDefVec::const_iterator end  =
															definition->mShadowMapTexDefinitions.end();

		while( itor != end )
		{
			CompositorChannel newChannel;

			String textureName = (itor->name + IdString( id )).getFriendlyText();
			if( itor->formatList.size() == 1 )
			{
				//Normal RT
				TexturePtr tex = TextureManager::getSingleton().createManual( textureName,
												ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
												TEX_TYPE_2D, itor->width, itor->height, 0,
												itor->formatList[0], TU_RENDERTARGET, 0,
												itor->hwGammaWrite, itor->fsaa );
				RenderTexture* rt = tex->getBuffer()->getRenderTarget();
				newChannel.target = rt;
				newChannel.textures.push_back( tex );
			}
			else
			{
				//MRT
				MultiRenderTarget* mrt = mRenderSystem->createMultiRenderTarget( textureName );
				PixelFormatList::const_iterator pixIt = itor->formatList.begin();
				PixelFormatList::const_iterator pixEn = itor->formatList.end();

				newChannel.target = mrt;

				while( pixIt != pixEn )
				{
					size_t rtNum = pixIt - itor->formatList.begin();
					TexturePtr tex = TextureManager::getSingleton().createManual(
												textureName + StringConverter::toString( rtNum ),
												ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
												TEX_TYPE_2D, itor->width, itor->height, 0,
												*pixIt, TU_RENDERTARGET, 0, itor->hwGammaWrite,
												itor->fsaa );
					RenderTexture* rt = tex->getBuffer()->getRenderTarget();
					mrt->bindSurface( rtNum, rt );
					newChannel.textures.push_back( tex );
					++pixIt;
				}
			}

			mLocalTextures.push_back( newChannel );

			++itor;
		}

		// Shadow Nodes don't have input and global texture should've been created by
		// the time we get created. Therefore, we can safely initialize now as our
		// output may be used in regular nodes and we're created on-demand (as soon
		// as a Node discovers it needs us for the first time, we get created)
		initializePasses();
	}
	//-----------------------------------------------------------------------------------
	CompositorShadowNode::~CompositorShadowNode()
	{
	}
	//-----------------------------------------------------------------------------------
}