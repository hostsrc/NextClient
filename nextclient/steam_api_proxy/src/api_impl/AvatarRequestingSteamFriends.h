#pragma once

// Fetches Steam avatars of other players on the server (not only friends) for the scoreboard.
//
// The CS 1.6 client (client.dll, steam_legacy 8684 and the 25th anniversary build alike) shows a
// player's avatar with ISteamFriends::GetMediumFriendAvatar(*sid) and redraws when the returned
// image handle changes. It never calls RequestUserInformation, so it only ever sees avatars Steam
// already holds. Steam used to fill that cache for players on the same server by itself; since
// about 18 Nov 2026 it no longer does, so only friends' avatars show
// (https://github.com/ValveSoftware/halflife/issues/4043).
//
// This wrapper passes every ISteamFriends call through unchanged, except that an avatar lookup that
// returns 0 ("Steam has nothing for this user") also asks Steam to fetch that user's persona data,
// at most once per user every 30 seconds. Steam answers within about a second; the scoreboard's next
// pass then gets a real handle and draws the avatar. Tested by hand on 22 Nov 2026: one request per
// player was enough for their avatar to appear in the stock scoreboard.

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include <steam/steam_api.h>

class AvatarRequestingSteamFriends final : public ISteamFriends
{
public:
    explicit AvatarRequestingSteamFriends(ISteamFriends* real) : real_(real) {}

    ISteamFriends* Real() const { return real_; }

    // --- generated: every method forwards to the real interface (avatar getters also request) ---
    const char * GetPersonaName() override { return real_->GetPersonaName(); }
    SteamAPICall_t SetPersonaName(const char *pchPersonaName) override { return real_->SetPersonaName(pchPersonaName); }
    EPersonaState GetPersonaState() override { return real_->GetPersonaState(); }
    int GetFriendCount(int iFriendFlags) override { return real_->GetFriendCount(iFriendFlags); }
    CSteamID GetFriendByIndex(int iFriend, int iFriendFlags) override { return real_->GetFriendByIndex(iFriend, iFriendFlags); }
    EFriendRelationship GetFriendRelationship(CSteamID steamIDFriend) override { return real_->GetFriendRelationship(steamIDFriend); }
    EPersonaState GetFriendPersonaState(CSteamID steamIDFriend) override { return real_->GetFriendPersonaState(steamIDFriend); }
    const char * GetFriendPersonaName(CSteamID steamIDFriend) override { return real_->GetFriendPersonaName(steamIDFriend); }
    bool GetFriendGamePlayed(CSteamID steamIDFriend, FriendGameInfo_t *pFriendGameInfo) override { return real_->GetFriendGamePlayed(steamIDFriend, pFriendGameInfo); }
    const char * GetFriendPersonaNameHistory(CSteamID steamIDFriend, int iPersonaName) override { return real_->GetFriendPersonaNameHistory(steamIDFriend, iPersonaName); }
    bool HasFriend(CSteamID steamIDFriend, int iFriendFlags) override { return real_->HasFriend(steamIDFriend, iFriendFlags); }
    int GetClanCount() override { return real_->GetClanCount(); }
    CSteamID GetClanByIndex(int iClan) override { return real_->GetClanByIndex(iClan); }
    const char * GetClanName(CSteamID steamIDClan) override { return real_->GetClanName(steamIDClan); }
    const char * GetClanTag(CSteamID steamIDClan) override { return real_->GetClanTag(steamIDClan); }
    bool GetClanActivityCounts(CSteamID steamIDClan, int *pnOnline, int *pnInGame, int *pnChatting) override { return real_->GetClanActivityCounts(steamIDClan, pnOnline, pnInGame, pnChatting); }
    SteamAPICall_t DownloadClanActivityCounts(CSteamID *psteamIDClans, int cClansToRequest) override { return real_->DownloadClanActivityCounts(psteamIDClans, cClansToRequest); }
    int GetFriendCountFromSource(CSteamID steamIDSource) override { return real_->GetFriendCountFromSource(steamIDSource); }
    CSteamID GetFriendFromSourceByIndex(CSteamID steamIDSource, int iFriend) override { return real_->GetFriendFromSourceByIndex(steamIDSource, iFriend); }
    bool IsUserInSource(CSteamID steamIDUser, CSteamID steamIDSource) override { return real_->IsUserInSource(steamIDUser, steamIDSource); }
    void SetInGameVoiceSpeaking(CSteamID steamIDUser, bool bSpeaking) override { real_->SetInGameVoiceSpeaking(steamIDUser, bSpeaking); }
    void ActivateGameOverlay(const char *pchDialog) override { real_->ActivateGameOverlay(pchDialog); }
    void ActivateGameOverlayToUser(const char *pchDialog, CSteamID steamID) override { real_->ActivateGameOverlayToUser(pchDialog, steamID); }
    void ActivateGameOverlayToWebPage(const char *pchURL) override { real_->ActivateGameOverlayToWebPage(pchURL); }
    void ActivateGameOverlayToStore(AppId_t nAppID, EOverlayToStoreFlag eFlag) override { real_->ActivateGameOverlayToStore(nAppID, eFlag); }
    void SetPlayedWith(CSteamID steamIDUserPlayedWith) override { real_->SetPlayedWith(steamIDUserPlayedWith); }
    void ActivateGameOverlayInviteDialog(CSteamID steamIDLobby) override { real_->ActivateGameOverlayInviteDialog(steamIDLobby); }
    int GetSmallFriendAvatar(CSteamID steamIDFriend) override { return RequestIfMissing(steamIDFriend, real_->GetSmallFriendAvatar(steamIDFriend)); }
    int GetMediumFriendAvatar(CSteamID steamIDFriend) override { return RequestIfMissing(steamIDFriend, real_->GetMediumFriendAvatar(steamIDFriend)); }
    int GetLargeFriendAvatar(CSteamID steamIDFriend) override { return RequestIfMissing(steamIDFriend, real_->GetLargeFriendAvatar(steamIDFriend)); }
    bool RequestUserInformation(CSteamID steamIDUser, bool bRequireNameOnly) override { return real_->RequestUserInformation(steamIDUser, bRequireNameOnly); }
    SteamAPICall_t RequestClanOfficerList(CSteamID steamIDClan) override { return real_->RequestClanOfficerList(steamIDClan); }
    CSteamID GetClanOwner(CSteamID steamIDClan) override { return real_->GetClanOwner(steamIDClan); }
    int GetClanOfficerCount(CSteamID steamIDClan) override { return real_->GetClanOfficerCount(steamIDClan); }
    CSteamID GetClanOfficerByIndex(CSteamID steamIDClan, int iOfficer) override { return real_->GetClanOfficerByIndex(steamIDClan, iOfficer); }
    uint32 GetUserRestrictions() override { return real_->GetUserRestrictions(); }
    bool SetRichPresence(const char *pchKey, const char *pchValue) override { return real_->SetRichPresence(pchKey, pchValue); }
    void ClearRichPresence() override { real_->ClearRichPresence(); }
    const char * GetFriendRichPresence(CSteamID steamIDFriend, const char *pchKey) override { return real_->GetFriendRichPresence(steamIDFriend, pchKey); }
    int GetFriendRichPresenceKeyCount(CSteamID steamIDFriend) override { return real_->GetFriendRichPresenceKeyCount(steamIDFriend); }
    const char * GetFriendRichPresenceKeyByIndex(CSteamID steamIDFriend, int iKey) override { return real_->GetFriendRichPresenceKeyByIndex(steamIDFriend, iKey); }
    void RequestFriendRichPresence(CSteamID steamIDFriend) override { real_->RequestFriendRichPresence(steamIDFriend); }
    bool InviteUserToGame(CSteamID steamIDFriend, const char *pchConnectString) override { return real_->InviteUserToGame(steamIDFriend, pchConnectString); }
    int GetCoplayFriendCount() override { return real_->GetCoplayFriendCount(); }
    CSteamID GetCoplayFriend(int iCoplayFriend) override { return real_->GetCoplayFriend(iCoplayFriend); }
    int GetFriendCoplayTime(CSteamID steamIDFriend) override { return real_->GetFriendCoplayTime(steamIDFriend); }
    AppId_t GetFriendCoplayGame(CSteamID steamIDFriend) override { return real_->GetFriendCoplayGame(steamIDFriend); }
    SteamAPICall_t JoinClanChatRoom(CSteamID steamIDClan) override { return real_->JoinClanChatRoom(steamIDClan); }
    bool LeaveClanChatRoom(CSteamID steamIDClan) override { return real_->LeaveClanChatRoom(steamIDClan); }
    int GetClanChatMemberCount(CSteamID steamIDClan) override { return real_->GetClanChatMemberCount(steamIDClan); }
    CSteamID GetChatMemberByIndex(CSteamID steamIDClan, int iUser) override { return real_->GetChatMemberByIndex(steamIDClan, iUser); }
    bool SendClanChatMessage(CSteamID steamIDClanChat, const char *pchText) override { return real_->SendClanChatMessage(steamIDClanChat, pchText); }
    int GetClanChatMessage(CSteamID steamIDClanChat, int iMessage, void *prgchText, int cchTextMax, EChatEntryType * arg4, CSteamID * arg5) override { return real_->GetClanChatMessage(steamIDClanChat, iMessage, prgchText, cchTextMax, arg4, arg5); }
    bool IsClanChatAdmin(CSteamID steamIDClanChat, CSteamID steamIDUser) override { return real_->IsClanChatAdmin(steamIDClanChat, steamIDUser); }
    bool IsClanChatWindowOpenInSteam(CSteamID steamIDClanChat) override { return real_->IsClanChatWindowOpenInSteam(steamIDClanChat); }
    bool OpenClanChatWindowInSteam(CSteamID steamIDClanChat) override { return real_->OpenClanChatWindowInSteam(steamIDClanChat); }
    bool CloseClanChatWindowInSteam(CSteamID steamIDClanChat) override { return real_->CloseClanChatWindowInSteam(steamIDClanChat); }
    bool SetListenForFriendsMessages(bool bInterceptEnabled) override { return real_->SetListenForFriendsMessages(bInterceptEnabled); }
    bool ReplyToFriendMessage(CSteamID steamIDFriend, const char *pchMsgToSend) override { return real_->ReplyToFriendMessage(steamIDFriend, pchMsgToSend); }
    int GetFriendMessage(CSteamID steamIDFriend, int iMessageID, void *pvData, int cubData, EChatEntryType *peChatEntryType) override { return real_->GetFriendMessage(steamIDFriend, iMessageID, pvData, cubData, peChatEntryType); }
    SteamAPICall_t GetFollowerCount(CSteamID steamID) override { return real_->GetFollowerCount(steamID); }
    SteamAPICall_t IsFollowing(CSteamID steamID) override { return real_->IsFollowing(steamID); }
    SteamAPICall_t EnumerateFollowingList(uint32 unStartIndex) override { return real_->EnumerateFollowingList(unStartIndex); }

private:
    // Returns avatar_handle unchanged. If it is 0, asks Steam (once per user per kRetryAfter) to
    // download the user's persona data, which includes the avatar.
    int RequestIfMissing(CSteamID user, int avatar_handle);

    static constexpr std::chrono::seconds kRetryAfter{30};
    static constexpr size_t kMaxTrackedUsers = 4096;   // a server holds at most 32 players

    ISteamFriends* real_;
    std::mutex mutex_;
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> last_request_;
};
