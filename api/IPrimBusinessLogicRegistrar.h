
#pragma once

"IPrimBusinessLogic.h"

class IPrimBusinessLogicRegistrar
{
public:

    // create a default chain of business logic, i.e.
    // 
    //   CurrentToolModeLogic ---> Department Logic ---> Company Logic ---> maya-usd Defaults
    // 
    void pushBusinessLogic(IPrimBusinessLogic* logic)
    {
        logic->setNext(m_defaultBusinessLogic);
        m_defaultBusinessLogic = logic;
    }

    // remove top most 
    bool popBusinessLogic()
    {
        if(!m_defaultBusinessLogic)
            return false;
        m_defaultBusinessLogic = m_defaultBusinessLogic->next();
        return m_defaultBusinessLogic;
    }

    // first look to see if there is some logic registered against its kind, 
    // and if that fails, return 
    IPrimBusinessLogic* defaultBusinessLogic() const
    {
        return m_defaultBusinessLogic;
    }

private:
    IPrimBusinessLogic* m_defaultBusinessLogic = 0;
};