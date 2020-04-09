#pragma once
#include "../api/IPrimBusinessLogicRegistrar.h"

/// 
///  An example of the kind of interface we could end up using on the maya-usd UFE integration side. 
///  Effectively we are either going to:
///
///    a) ask for an default translate/rotate/scale xform op (and associated meta data we may need for the GUI)
///    b) ask for meta data for a specific xform op. from that we will know what we can do with it 
///       (edit time samples, but not the default, etc, etc)   
///   
///  Worth noting that here the calls to request default values, always end up calling classifyXformOp. 
///  That's simply to ensure all attribute requests come through a single interceptable point.   
/// 
class PrimBusinessLogic
{
    IPrimBusinessLogicRegistrar* m_registrar = 0;
public:

    using IPrimBusinessLogic::XformOpInfo;

    /// it's assumed 'logic' will be the pointer returned from the global ufe IPrimBusinessLogicRegistrar::defaultBusinessLogic()
    PrimBusinessLogic(IPrimBusinessLogicRegistrar* registrar = getSomeGlobalRegistrar())
        : m_registrar(registrar) {}

    /// given some random attribute on a prim, what does the studio business logic say we can do with it?
    AttrInfo classifyAttr(const UsdPrim& prim, const UsdAttribute& attr)
    {
        return m_registrar->defaultBusinessLogic()->classifyAttr(prim, attr);
    }

    /// given some random xform Op from a random xform, am I able to write to the value, 
    /// should I be looking at time samples or the default, etc etc.
    XformOpInfo classifyXformOp(const UsdGeomXformable& xform, const UsdGeomXformOp& xformOP)
    {
        return m_registrar->defaultBusinessLogic()->classifyXformOp(xform, xformOP);
    }

    /// \brief  grab (or create)
    XformOpInfo defaultRotateOpInfo(const UsdGeomXformable& xform, bool create = true) const
    {
        UsdGeomXformOp op = m_registrar->defaultBusinessLogic()->defaultRotateOp(xform);
        if(!op && create)
        {
            op = m_registrar->defaultBusinessLogic()->createRotateOp(xform);
        }
        return !op ? XformOpInfo() : m_registrar->defaultBusinessLogic()->classifyXformOp(xform, op);
    }
    
    /// \brief  given some xform, defer to the prim business logic handler of the studio, and ask for which rotation value
    ///         should be assigned to the rotate manip when the user presses 'E'
    XformOpInfo defaultTranslateOpInfo(const UsdGeomXformable& xform, bool create = true) const
    {
        UsdGeomXformOp op = m_registrar->defaultBusinessLogic()->defaultTranslateOp(xform);
        if(!op && create)
        {
            op = m_registrar->defaultBusinessLogic()->createTranslateOp(xform);
        }
        return !op ? XformOpInfo() : m_registrar->defaultBusinessLogic()->classifyXformOp(xform, op);
    }
    
    /// \brief  given some xform, defer to the prim business logic handler of the studio, and ask for which scale value
    ///         should be assigned to the scale manip when the user presses 'R'
    XformOpInfo defaultScaleOpInfo(const UsdGeomXformable& xform, bool create = true) const
    {
        UsdGeomXformOp op = m_registrar->defaultBusinessLogic()->defaultScaleOp(xform);
        if(!op && create)
        {
            op = m_registrar->defaultBusinessLogic()->createScaleOp(xform);
        }
        return !op ? XformOpInfo() : m_registrar->defaultBusinessLogic()->classifyXformOp(xform, op);
    }
};