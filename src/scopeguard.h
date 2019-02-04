
#pragma once

class ScopeGuard
{
public:
    explicit ScopeGuard(std::function<void()> onExitScope)
        : onExitScope(onExitScope) {}

    ~ScopeGuard()
    {
        if (!mDismissed)
            onExitScope();
    }

    void Dismiss()
    {
        mDismissed = true;
    }

private:
    std::function<void()> onExitScope;
    bool mDismissed = false;
};

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)
#define ON_SCOPE_EXIT(callback) ScopeGuard SCOPEGUARD_LINENAME(EXIT, __LINE__)([&]{ callback; })
