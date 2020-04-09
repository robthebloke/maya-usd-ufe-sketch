
#pragma once
#include "IAttrBusinessLogic.h"

#include <cstdio>
#include <cstdint>

/// broadly speaking the business logic is going to break down into two main areas. 
/// 
/// 1: Given some xform op, query business logic of studio...
///----------------------------------------------------------
/// In order to manipulate a given xform op, we first need to know the following info:
///
///  * Are we editing the default or sampled time?
///  * Are we able to invert the coordinate frames?
///  * Is this a scale, rotate, or translate operation?
///  * If it's a translate op, should we display the edit pivots manips?
///  * 
/// 
/// 2: Connecting the TRS tools to a sensible xform op
///-----------------------------------------------------------
/// When the user has either the translate, scale, or rotate tools active, and selects a UFE/USD prim,
///   
class IPrimBusinessLogic
{
    IPrimBusinessLogic* m_next = 0;
public:

    //-----------------------------------------------------------------------------------------------------------------------
    /// The business logic is ordered in a chain. If the first business logic in the chain handles the request, cool. If not,
    /// the request is passed down the chain. 
    //-----------------------------------------------------------------------------------------------------------------------

    /// the business logics work as a stack. If a logic intercepts some request, it handles it. 
    /// otherwise it passes the request along to the next in the chain. 
    IPrimBusinessLogic* next() const
        { return m_next; }

    /// good software design this is not.
    void setNext(IPrimBusinessLogic* next)
        { m_next = next; }

    enum AttrFlag
    {
        kUseDefaultTime = 1 << 0, ///< read/write attribute using the default time
        kLock = 1 << 1,  ///< if the clients business logic returns locked, do not modify attribute 

        /* there may be others we need, e.g. for unit conversion, can we create anim curve for attr, etc */

        kLastAttrFLag = 1 << 2
    };

    /// package together all the flags and additional info provided by the studio business logic 
    /// that determine what can/can't happen to any given attribute (i.e. should we be reading from the default time,
    /// should the attr be locked for editing, etc, etc)
    struct AttrInfo
    {
        typedef uint32_t AttrFlags;

        // flags returns by studios business logic
        AttrFlags flags;

        bool setFlag(AttrFlags flag)
            { flags |= flag; }

        bool clearFlag(AttrFlags flag)
            { flags &= ~flag; }

        bool flagSet(AttrFlags flag) const
            { return (flags & ~flag) != 0; }
    };
 
    //-----------------------------------------------------------------------------------------------------------------------
    /// Given some xformOp that maya-usd wants to modify/query, we need to request  
    //-----------------------------------------------------------------------------------------------------------------------

    // A set of additional flags, mostly generated automatically by inspecting the xform ops.
    enum XformOpFlag
    {
        kCoordinateFrameNonInvertable = kLastAttrFLag << 0, ///< If we can't invert the previous transform ops in the stack, then we can't modify in TRS tools 
        kParentFrameNonInvertable = kLastAttrFLag << 1, ///< If the coordinate frame cannot be inverted, we can't modify the values
        kRotationOp = kLastAttrFLag << 2, ///< this xform op is a rotation op
        kScaleOp = kLastAttrFLag << 3, ///< this xform op is a scale op
        kTranslateOp = kLastAttrFLag << 4, ///< this xform op is a translation op
        kEditPivotManip = kLastAttrFLag << 5, ///< display the edit pivot manip whilst treating as a translate op
        kLastXformOpFlag = kLastAttrFLag << 6
    };

    /// all of the info we need from the business logic so that we know what we 
    /// can and can't do with this xform op (i.e. set, read from default value, etc) 
    /// 
    struct XformOpInfo : public AttrInfo 
    {
        MMatrix coordinateFrame; ///< we shoula be able to generate generically - just accumulate a matrix along the stack
        MMatrix parentFrame; ///< the parent transform
        UsdGeomXformOp xformOp;
        UsdGeomXformable xform;

        bool locked() const {
            return flagSet(kLock) || 
                   flagSet(kCoordinateFrameNonInvertable) || 
                   flagSet(kParentFrameNonInvertable); 
        }

        bool isRotateOp() const
            { return flagSet(kRotationOp); }
            
        bool isScaleOp() const
            { return flagSet(kScaleOp); }
            
        bool isTranslateOp() const
            { return flagSet(kTranslateOp); }
            
        bool isEditingPivots() const
            { return flagSet(kTranslateOp) && flagSet(kEditPivotManip); }
    };
    
    /// override to determine what can/can't be done with a specific attribute 
    virtual AttrInfo classifyAttr(const UsdPrim& prim, const UsdAttribute& attr)
    {
        // some sensible maya default (possibly checking a schema for prim type, or other mechanism)
        return AttrInfo { 0 };
    }
    
    /// *possibly* doesn't need to be virtual, just might be handy if it was 
    /// (e.g. if i want to enable custom handling for xform attrs only).  
    virtual XformOpInfo classifyXformOp(const UsdGeomXformable& xform, const UsdGeomXformOp& xformOP)
    {
        // query the attribute info 
        XformOpInfo info = classifyAttr(xform.GetPrim(), xformOP.GetAttribute()); 

        // append additional data for transform ops
        info.xform = xform;
        info.xformOP = xformOP;

        // probably not going to be done here, but we need to determoine whether the frame is invertable,
        // otherwise some of the transform modes can't be applied?
        info.computeCoordinateFrames();
        return info;
    }

    //-----------------------------------------------------------------------------------------------------------------------
    /// Given some prim, these methods return the default translate/scale/rotate values that UFE should manipulate. 
    /// They should not create the attributes!
    //-----------------------------------------------------------------------------------------------------------------------

    /// \brief  given some xform, defer to the prim business logic handler of the studio, and ask for which translation value
    ///         should be assigned to the translate manip when the user presses 'W'. 
    virtual UsdGeomXformOp defaultRotateOp(const UsdGeomXformable& xform)
    {
        // the common transform api simply uses the name "", and simply says if it's "xyz", use it. 
        // the default fallback is simply any xformOp that matches the name "xformOp:rotate###".
        return findRotateOpWithNoName(xform);  
    }
    
    /// \brief  given some xform, defer to the prim business logic handler of the studio, and ask for which rotation value
    ///         should be assigned to the rotate manip when the user presses 'E'
    virtual UsdGeomXformOp defaultTranslateOp(const UsdGeomXformable& xform)
    {
        // just look for the translate op that has the name "". 
        return findTranslateOpWithNoName(xform);  
    }
    
    /// \brief  given some xform, defer to the prim business logic handler of the studio, and ask for which scale value
    ///         should be assigned to the scale manip when the user presses 'R'
    virtual UsdGeomXformOp defaultScaleOp(const UsdGeomXformable& xform)
    {
        // just look for the scale op that has the name "". 
        return findScaleOpWithNoName(xform);  
    }

    //-----------------------------------------------------------------------------------------------------------------------
    /// Should maya-usd need to insert a new translate/scale/rotate op (because the op was not found), maya-usd will
    /// call these methods, and it's up to the studio's business logic to do the right thing. 
    //-----------------------------------------------------------------------------------------------------------------------

    /// \brief  If the call to defaultRotateOp fails to find an op, we need to create a new op, 
    ///         and insert it into the correct place within the chain of operations. 
    ///         The client studio can then make the decision whether or not to create the op, 
    ///         and into which layer that gets written.
    virtual UsdGeomXformOp createRotateOp(UsdGeomXformable& xform)
    {
        // Whilst I think it would be possible to create some lookup tables based on prim type, 
        // and/or the kind, to some lovely table that defined the order of ops used (An xform op schema if you will),
        // I actually don't think that will cover all of the cases. 
        // 
        // Business logic of the studio tools may want to say: "this tool is in cache preview mode, disable all edits"
        // 
        // Querying some xform-op-schema database should probably be the default in the core maya-usd implementation?
        return createRotateOpAndInsertIntoXformStack(xform);  
    }
    
    /// \brief  create the translate op in the correct place in the stack
    virtual UsdGeomXformOp createTranslateOp(const UsdGeomXformable& xform)
    {
        return createTranslateOpAndInsertIntoXformStack(xform);  
    }
    
    /// \brief  create the scale op in the correct place in the stack
    virtual UsdGeomXformOp createScaleOp(const UsdGeomXformable& xform)
    {
        return createScaleOpAndInsertIntoXformStack(xform);  
    }
};

