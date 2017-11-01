
#pragma once

class ScopeGuard
{
public:
    explicit ScopeGuard(std::function<void()> onExitScope)
        : onExitScope_(onExitScope) {}

    ~ScopeGuard()
    {
        if (!dismissed_)
        {
            onExitScope_();
        }
    }

    void Dismiss()
    {
        dismissed_ = true;
    }

private:
    std::function<void()> onExitScope_;
    bool dismissed_ = false;
};

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)
#define ON_SCOPE_EXIT(callback) ScopeGuard SCOPEGUARD_LINENAME(EXIT, __LINE__)([&]{ callback; })
