/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#include <string.h>
#if defined(__linux) || defined(__APPLE__)
#include <grp.h>
#include <pwd.h>
#endif
#include "config.hpp"
#include "exception.hpp"
#include "path.hpp"
#include "process.hpp"
#include "user.hpp"

using namespace std;
using namespace c4s;

#if defined(__linux) || defined(__APPLE__)
// -------------------------------------------------------------------------------------------------
/**  \param name_in name of the user.
 */
c4s::user::user(const char* name_in)
{
    if (name_in)
        name = name_in;
    system = false;
    uid = -1;
    gid = -1;
    read();
}

// -------------------------------------------------------------------------------------------------
/** \param name_in user name
  \param group_in user's primary group name
  \param system_in if true then this is a system user.
 */
c4s::user::user(const char* name_in, const char* group_in, bool system_in)
{
    if (name_in)
        name = name_in;
    if (group_in)
        group = group_in;
    system = system_in;
    uid = -1;
    gid = -1;
    read();
}

// -------------------------------------------------------------------------------------------------
/**  Use NULL values for parameters to specify empty values. If user exists his uid and gid are read
    from system. If home and shell parameters are specified then they will replace the values read
    from the system. Use create() function to store values to system.
    \param name_in user name
    \param group_in user's primary group name
    \param system_in if true then this is a system user.
    \param shell_in User's login shell.
    \param home_in User's home directory.
    \param GROUPS_in User's additional groups, comma separated list.
*/
c4s::user::user(const char* name_in,
                const char* group_in,
                bool system_in,
                const char* home_in,
                const char* shell_in,
                const char* GROUPS_in)
{
    if (name_in)
        name = name_in;
    if (group_in)
        group = group_in;
    system = system_in;
    uid = -1;
    gid = -1;
    read();
    if (GROUPS_in)
        GROUPS = GROUPS_in;
    if (home_in)
        home = home_in;
    if (shell_in)
        shell = shell_in;
}

// -------------------------------------------------------------------------------------------------
c4s::user::user(const user& orig)
{
    home = orig.home;
    GROUPS = orig.GROUPS;
    shell = orig.shell;

    system = orig.system;
    name = orig.name;
    group = orig.group;
    uid = orig.uid;
    gid = orig.gid;
}

// -------------------------------------------------------------------------------------------------
/**  All user attributes will be reset. Throws exception if ids do not exist.
  \param id_u User id to set.
  \param id_g Group id to set.
*/
void
c4s::user::set(int id_u, int id_g)
{
    ostringstream os;
    struct passwd* pwd = getpwuid(id_u);
    if (pwd) {
        uid = id_u;
        name = pwd->pw_name;
        if (pwd->pw_dir)
            home = pwd->pw_dir;
        if (pwd->pw_shell)
            shell = pwd->pw_shell;
    } else {
        os << "User with id " << id_u << " does not exist.";
        throw c4s_exception(os.str());
    }

    struct group* grp = getgrgid(id_g);
    if (grp) {
        if ((unsigned int)id_g == pwd->pw_gid) {
            gid = id_g;
            group = grp->gr_name;
            return;
        }
        // search rest of the members
        char** member;
        for (member = grp->gr_mem; *member; member++) {
            if (!strcmp(*member, name.c_str())) {
                gid = id_g;
                group = grp->gr_name;
                return;
            }
        }
    }
    // No luck
    os << "User " << name << " is not member of group " << id_g;
    throw c4s_exception(os.str());
}

// -------------------------------------------------------------------------------------------------
void
c4s::user::dump(ostream& os)
{
    os << "user == " << name << "(" << uid << ") / " << group << "(" << gid << ")";
    if (system)
        os << " [system]";
    if (!shell.empty())
        os << " [S:" << shell << "]";
    if (!home.empty())
        os << " [H:" << home << "]";
    if (!GROUPS.empty())
        os << " [G:" << GROUPS << "]\n";
    else
        os << '\n';
}

// -------------------------------------------------------------------------------------------------
/** Non-zero values tell the status:
    1 = user or group does not exist
    2 = home directory is different
    3 = shell is different
    4 = user does not belong to primary group
    5 = user is not member of additional groups
  \param refresh If true then the user information is read from the system before validation.
  \retval int Zero if user exist in system as it is in this object.
 */
int
c4s::user::status(bool refresh)
{
    if (refresh)
        read();
#ifdef C4S_DEBUGTRACE
    cout << "user::status() | ";
    dump(cout);
#endif
    // Check that defined user and group existed.
    if ((!name.empty() && uid == -1) || (!group.empty() && gid == -1)) {
        return 1;
    }
    // Check the home and shell
    struct passwd* pwd = getpwuid(uid);
    if (!home.empty()) {
        // Discard the trailing slash if specified
        size_t hl = home.size();
        if (home.at(hl - 1) == C4S_DSEP)
            hl--;
        if (home.compare(0, hl, pwd->pw_dir, hl))
            return 2;
    }
    if (!shell.empty()) {
        if (shell.compare(pwd->pw_shell))
            return 3;
    }
    if (gid >= 0) {
        // Check if user belongs to the given group
        if (pwd && gid != (int)pwd->pw_gid)
            return 4;
    }
    if (!GROUPS.empty()) {
        size_t start = 0, end;
        struct group* grp;
        string cgroup;
        while (start != string::npos) {
            end = GROUPS.find(',', start);
            cgroup = GROUPS.substr(start, end);
            grp = getgrnam(cgroup.c_str());
            if (!grp) {
                ostringstream err;
                err << "user::status - user additional group '" << cgroup
                    << "' does not exist in the system.";
                throw c4s_exception(err.str());
            }
            char** member;
            for (member = grp->gr_mem; *member; member++) {
                if (!name.compare(*member))
                    break;
            }
            if (*member == 0)
                return 5;
            start = end;
            if (start != string::npos)
                start++;
        }
    }
    return 0;
}

// -------------------------------------------------------------------------------------------------
/** User and group names should have been defined before calling this function.
    Note! The resulting uid/gid combination might not exist in the system. Use create() to enforce
   it.
*/
void
c4s::user::read()
{
    struct passwd* pwd = 0;
    struct group* grp = 0;

    if (!name.empty()) {
        pwd = getpwnam(name.c_str());
        if (pwd) {
            uid = pwd->pw_uid;
            // \TODO: Fix the constant below. We should read the system user limit from
            // configuration file.
            if (uid < 1000)
                system = true;
            if (group.empty()) {
                // Read user's default group from system.
                gid = pwd->pw_gid;
                grp = getgrgid(gid);
                if (grp)
                    group = grp->gr_name;
                return;
            }
            if (pwd->pw_dir)
                home = pwd->pw_dir;
            if (pwd->pw_shell)
                shell = pwd->pw_shell;
        } else
            uid = -1;
    } else
        uid = -1;

    if (!group.empty()) {
        // Check first the primary group
        if (pwd) {
            grp = getgrgid(pwd->pw_gid);
            if (grp) {
                // Make sure the name matches
                if (!group.compare(grp->gr_name)) {
                    gid = pwd->pw_gid;
                    return;
                }
            }
        }
        // Check the rest of the groups
        grp = getgrnam(group.c_str());
        if (grp)
            gid = grp->gr_gid;
        else
            gid = -1;
    } else
        gid = -1;
}

// -------------------------------------------------------------------------------------------------
/** Tries to match given user and group id to this user. It is considered a match if user belongs
  to the given group even if user's current group id does not match the given id.
  \param id_u User id to check
  \param id_g Group id to check
  \retval bool True on match, false if not.
*/
bool
c4s::user::match(int id_u, int id_g)
{
    if (!is_ok())
        return false;
    if (uid >= 0 && id_u != uid)
        return false;
    if (id_g == gid)
        return true;
    if (uid == -1)
        return false;
    // Search the members of the given group if user belongs there
    struct group* grp = getgrgid(id_g);
    if (!grp)
        return false;
    char** member;
    for (member = grp->gr_mem; *member; member++) {
        if (!strcmp(*member, name.c_str()))
            return true;
    }
    return false;
}

// -------------------------------------------------------------------------------------------------
//! Fills this object with information for the current process owner.
user
c4s::user::get_current()
{
    user tmp;
    tmp.uid = geteuid();
    struct passwd* pw = getpwuid(tmp.uid);
    if (pw)
        tmp.name = pw->pw_name;
    if (tmp.uid < 1000)
        tmp.system = true;
    tmp.gid = getegid();
    struct group* gr = getgrgid(tmp.gid);
    if (gr)
        tmp.group = gr->gr_name;
    return tmp;
}

// -------------------------------------------------------------------------------------------------
/** Function will call status() internally and will not do anything if the user already exists in
  system as it is in memory.
  \param append_groups If true then the GROUPS list is appended to the system i.e. -a parameter is
  used with usermod.
 */
void
c4s::user::create(bool append_groups)
{
    struct passwd* pwd;
    struct group* grp;

    if (status() == 0) {
        return;
    }
    bool nzrv_restore = process::nzrv_exception;
    process::nzrv_exception = false;

    // Add group if it does not exist alread, and if name!=group.
    // In latter case we allow adduser-command to create group automatially.
    stringstream elog;
    elog << "user::create ";
    if (!group.empty() && gid == -1 &&
        (name.empty() || (name.empty() == false && name.compare(group)))) {
        elog << "Adding group " << group << ": ";
        process groupadd("groupadd", "-f", &elog);
        if (system)
            groupadd += "--system";
        groupadd += group;
        if (groupadd()) {
            process::nzrv_exception = nzrv_restore;
            throw c4s_exception(elog.str());
        }
        // We must read back the created gid
        grp = getgrnam(group.c_str());
        if (grp)
            gid = grp->gr_gid;
        else {
            process::nzrv_exception = nzrv_restore;
            elog << "\nuser::create - Error: created group cannot be found:" << group;
            throw c4s_exception(elog.str());
        }
    }
    if (name.empty()) {
        process::nzrv_exception = nzrv_restore;
        return;
    }
    elog.str("");

    // If user does not exist: add him/her + groups at the same time
    if (uid == -1) {
        elog << "Adding user " << name << ": ";
        process useradd("useradd");
        if (system)
            useradd += "--system";
        if (gid >= 0) {
            useradd += "-g";
            useradd += group;
        } else if (!group.empty())
            useradd += "-U";
        if (!GROUPS.empty()) {
            useradd += "-G";
            useradd += GROUPS;
        }
        if (!shell.empty()) {
            useradd += "-s";
            useradd += shell;
        }
        if (!home.empty()) {
            useradd += "-m -d";
            useradd += home;
        } else if (!system)
            useradd += "-m";

        useradd += name;
        useradd.pipe_to(&elog);
        if (useradd()) {
            process::nzrv_exception = nzrv_restore;
            throw c4s_exception(elog.str());
        }
        process::nzrv_exception = nzrv_restore;
        // We need to get back the created ids.
        pwd = getpwnam(name.c_str());
        if (!pwd) {
            elog << "Error - created user '" << name << "'is invalid. Status =" << status();
            throw c4s_exception(elog.str());
        }
        grp = getgrgid(pwd->pw_gid);
        if (!grp) {
            elog << "Error - users primary group not found after create.";
            throw c4s_exception(elog.str());
        }
        uid = pwd->pw_uid;
        gid = grp->gr_gid;
        return;
    } // if uid== -1

    elog << "Modifying user " << name << ": ";
    process usermod("usermod");
    pwd = getpwuid(uid);
    if (!pwd) {
        elog << " Error - user " << uid << " does not exist";
        throw c4s_exception(elog.str());
    }
    // Make sure the initial login group is correct.
    if (gid != (int)pwd->pw_gid) {
        ostringstream args;
        args << "-g " << gid;
        usermod += args.str();
    }
    // Add additional groups
    if (!GROUPS.empty()) {
        if (append_groups)
            usermod += "-a";
        usermod += "-G";
        usermod += GROUPS;
    }
    // Shell if it has been specified
    if (!shell.empty()) {
        usermod += "-s";
        usermod += shell;
    }
    if (!home.empty()) {
        usermod += "-d";
        usermod += home;
    }
    usermod += name;
    usermod.pipe_to(&elog);
    if (usermod()) {
        process::nzrv_exception = nzrv_restore;
        elog << "Error - Unable to change primary group for user:" << name;
        throw c4s_exception(elog.str());
    }
    process::nzrv_exception = nzrv_restore;
}

#endif
