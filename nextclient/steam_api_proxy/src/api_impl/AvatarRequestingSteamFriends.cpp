#include "AvatarRequestingSteamFriends.h"

int AvatarRequestingSteamFriends::RequestIfMissing(CSteamID user, int avatar_handle)
{
    // Only "nothing cached" needs a request. A positive handle is an image (Steam's default picture
    // while it downloads, then the real one); -1 means Steam is already loading it.
    if (avatar_handle != 0 || !user.IsValid() || !user.BIndividualAccount())
        return avatar_handle;

    const auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard lock(mutex_);
        if (last_request_.size() >= kMaxTrackedUsers)
            last_request_.clear();

        auto [entry, first_time] = last_request_.try_emplace(user.ConvertToUint64(), now);
        if (!first_time)
        {
            if (now - entry->second < kRetryAfter)
                return avatar_handle;
            entry->second = now;
        }
    }

    // false = we want the avatar too, not only the name.
    real_->RequestUserInformation(user, false);
    return avatar_handle;
}
