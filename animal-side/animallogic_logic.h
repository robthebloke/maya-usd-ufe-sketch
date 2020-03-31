#pragma once
#include "../api/IPrimBusinessLogicRegistrar.h"

/// Since all usd/ufe calls go through the 'classifyXformOp' method, we can simply 
/// intercept the calls, and force all attributes to be locked for editing. 
/// 
class ReadOnlyLogic
  : public IPrimBusinessLogic
{
public:

    // grab the info from 'next' in the logic chain. Modify the returned result so that we lock the value.
    XformOpInfo classifyXformOp(const UsdGeomXformable& xform, const UsdGeomXformOp& xformOP) override
    {
        // grab result from next up the chain
        XformOpInfo info = next()->classifyXformOp(xform, xformOP);
        // I don't care what the attribute is, you're not editing it!
        info.setFlag(XformOpInfo::kLock);
        // and you will only ever look at the playback cache
        info.clearFlag(XformOpInfo::kUseDefaultTime);
        return info;
    }
};


/// So this is a very simplistic overview of how the lighting department might want to implement
/// their business logic. 
/// 
class LightingDeptLogic
  : public IPrimBusinessLogic
{
public:

    // broadly speaking, at animal we tend to think in terms of 'what task needs to be done',
    // rather than someones specific job role. So taking a slightly simplistic version of 
    // how the lighting department might want to work, let's assume that these are the general
    // tasks that any given person in the lighting dept might have to accomplish:
    // 
    //  * animating lights within a complex shot
    //  * improving / updating lighting rigs. 
    //  * improving / updating lighting shaders.
    //  * some quick throw-away animation/render tests to see results of said improvements/updates
    //  * dealing with support requests from other depts (e.g. DI, anim, comp)
    // 
    enum CurrentMode
    {
        kEditingLightAnim, ///< lock all rig asset attrs, enable anim controls
        kEditingLightRig, ///< only display the default values, and unlock all rig asset attributes
        kEditingLightShaders, ///< only display the default values, and unlock all shader assets
        kTestingAnimation, ///< unlock everything, but write all changes to the 'temp_lighting_layer'
        kSupportTicket_AnimUser, ///< load the AnimDeptLogic, and override to allow editing of lighting assets
        kSupportTicket_DIUser, ///< load the DIDeptLogic, and override to allow editing of lighting assets
        kSupportTicket_CompUser ///< load the CompDeptLogic, and override to allow editing of lighting assets
    };

    CurrentMode m_mode = kEditingLightRig;

    // grab the info from next in the chain, and modify so we lock the value.
    XformOpInfo classifyXformOp(const UsdGeomXformable& xform, const UsdGeomXformOp& xformOP) override
    {
        // depending on the current user mode, allow/prevent various differing things.... 
        // It's probably they'd want to change modes at runtime. 
        switch(m_mode)
        {
        case kEditingLightAnim: return _classifyXformOp_editLightAnim(xform, xformOP);
        case kEditingLightRig: return _classifyXformOp_editLightRig(xform, xformOP);
        case kEditingLightShaders: return _classifyXformOp_editLightShaders(xform, xformOP);
        case kTestingAnimation: return _classifyXformOp_testingAnimation(xform, xformOP);
        case kSupportTicket_AnimUser: return _classifyXformOp_supportTicket_animUser(xform, xformOP);
        case kSupportTicket_DIUser: return _classifyXformOp_supportTicket_diUser(xform, xformOP);
        case kSupportTicket_CompUser: return _classifyXformOp_supportTicket_compUser(xform, xformOP);
        default: break;
        }
        return _classifyXformOp_editLightRig(xform, xformOP);;
    }
};

// we will probably need some form of plugin mechanism here. I want to be able to tell maya-usd
// that I have some business logic I want it to use, effectively a studio plugin to maya-usd.
MStatus initBusinessLogic()
{
    // by default, make everything read only
    IPrimBusinessLogicRegistrar::pushBusinessLogic(new ReadOnlyLogic);

    // create the correct logic based on the users department
    if(getEnv(USER_IS_IN_LIGHTING_DEPT))
    {
        IPrimBusinessLogicRegistrar::pushBusinessLogic(new LightingDeptBusinessLogic);
    }
    else
    if(getEnv(USER_IS_IN_ANIM_DEPT))
    {
        IPrimBusinessLogicRegistrar::pushBusinessLogic(new AnimDeptBusinessLogic);
    }
    else
    if(getEnv(USER_IS_IN_RIGGING_DEPT))
    {
        IPrimBusinessLogicRegistrar::pushBusinessLogic(new RiggingDeptBusinessLogic);
    }
}

