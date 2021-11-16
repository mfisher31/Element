#!/usr/bin/env python

from waflib.Configure import conf

def derive_env (ctx):
    env = ctx.env.derive()
    if ctx.host_is_windows():
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.dll'
    elif ctx.host_is_linux():
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.so'
    elif ctx.host_is_mac():
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.dylib'
    return env
