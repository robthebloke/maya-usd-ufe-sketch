
#pragma once


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

    /// the business logics work as a stack. If a logic intercepts some request, it handles it. 
    /// otherwise it passes the request along to the next in the chain. 
    IPrimBusinessLogic* next() const
        { return m_next; }

    /// good software design this is not.
    void setNext(IPrimBusinessLogic* next)
        { m_next = next; }

    // A set of flags, some of which are easy enough to generate automatically. 
    // kUseDefaultTIme, and kLock need to be provided by the studio and departments business logic. 
    enum XformOpFlag
    {
        kUseDefaultTime = 1 << 0, ///< read/write using the default time
        kLock = 1 << 1,  ///< if the clients business logic returns locked, do not modify attribute 
        kCoordinateFrameNonInvertable = 1 << 2, ///< If we can't invert the previous transform ops in the stack, then we can't modify in TRS tools 
        kParentFrameNonInvertable = 1 << 3, ///< If the coordinate frame cannot be inverted, we can't modify the values
        kRotationOp = 1 << 4, ///< this xform op is a rotation op
        kScaleOp = 1 << 5, ///< this xform op is a scale op
        kTranslateOp = 1 << 6, ///< this xform op is a translation op
        kEditPivotManip = 1 << 7, ///< display the edit pivot manip whilst treating as a translate op
    };

    typedef uint32_t XformOpFlags;

    /// all of the info we need from the business logic so that we know what we 
    /// can and can't do with this xform op (i.e. set, read from default value, etc) 
    /// 
    struct XformOpInfo 
    {
        XformOpFlags flags; ///< The flags that denote what we can/can't do with this xform op.  
        MMatrix coordinateFrame; ///< we shoula be able to generate generically - just accumulate a matrix along the stack
        MMatrix parentFrame; ///< the parent transform
        UsdGeomXformOp xformOp;
        UsdGeomXformable xform;

        bool flagSet(XformOpFlag flag) const
          { return (flags & flag) != 0; }

        bool locked() const {
            return flagSet(XformOpFlag::kLock) || 
                   flagSet(XformOpFlag::kCoordinateFrameNonInvertable) || 
                   flagSet(XformOpFlag::kParentFrameNonInvertable); 
        }

        bool isRotateOp() const
            { return flagSet(XformOpFlag::kRotationOp); }
            
        bool isScaleOp() const
            { return flagSet(XformOpFlag::kScaleOp); }
            
        bool isTranslateOp() const
            { return flagSet(XformOpFlag::kTranslateOp); }
            
        bool isEditingPivots() const
            { return flagSet(XformOpFlag::kTranslateOp) && flagSet(XformOpFlag::kEditPivotManip); }
    };
    
    /// override to determine what can/can't be done with 
    virtual XformOpInfo classifyXformOp(const UsdGeomXformable& xform, const UsdGeomXformOp& xformOP);

    //-----------------------------------------------------------------------------------------------------------------------
    /// If we are to support 
    /// 
    /// bool attachRotateManip(UsdGeomXformable& xform, IPrimBusinessLogic* logic, bool canCreate = true)
    /// {
    ///    //
    ///    // is the department or studio doing something different for rotation?
    ///    //  
    ///    if(UsdGeomXformOp op = logic->defaultRotateOp(xform))
    ///    {
    ///      XformOpInfo info = logic->classifyXformOp(xform, op);
    ///      if(!info.locked() && info.isTranslateOp())
    ///      {
    ///        setUpRotateManipOnAttr(xform, info);
    ///        return true;
    ///      }
    ///    }
    ///    else
    ///    if(canCreate)
    ///    {
    ///      //
    ///      // now query the department, or studio, if it can create a default rotation op
    ///      //
    ///      if(UsdGeomXformOp op = logic->createRotateOp(xform))
    ///      {
    ///        XformOpInfo info = logic->classifyXformOp(xform, op);
    ///        if(!info.locked() && info.isTranslateOp())
    ///        {
    ///          setUpRotateManipOnAttr(xform, info);
    ///          return true;
    ///        }
    ///      }
    ///    }
    ///    return false;
    /// }
    /// 
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

