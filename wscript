#!/usr/bin/env python

from subprocess import call
import os, sys
sys.path.insert (0, os.path.join (os.getcwd(), 'tools/waf'))
import element, juce, git

APPNAME         = element.APPNAME
VERSION         = element.VERSION
PLUGIN_VERSION  = element.PLUGIN_VERSION

VST3_PATH = 'libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK'

def options (opt):
    opt.load ("scripting compiler_c compiler_cxx ccache compiler juce depends host")

    opt.add_option ('--disable-ladspa', default=False, action='store_true', dest='no_ladspa', \
        help="Disable LADSPA plugin hosting")
    opt.add_option ('--disable-lv2', default=False, action='store_true', dest='no_lv2', \
        help="Disable LV2 plugin hosting")
    opt.add_option ('--with-vstsdk24', default='', dest='vstsdk24', type='string', 
        help='Path to vstsdk2.4 sources')
    opt.add_option ('--disable-vst', default=False, action='store_true', dest='no_vst', \
        help="Disable VST2 plugin hosting")
    opt.add_option ('--disable-vst3', default=False, action='store_true', dest='no_vst3', \
        help="Disable VST3 plugin hosting")
    
    opt.add_option ('--without-alsa', default=False, action='store_true', dest='no_alsa', \
        help="Build without ALSA support")
    opt.add_option ('--without-jack', default=False, action='store_true', dest='no_jack', \
        help="Build without JACK support")
    opt.add_option ('--with-asiosdk', default='', dest='asiosdk', type='string',
        help='Path to ASIO SDK sources')

    opt.add_option ('--bindir', default='', type='string', dest='bindir', \
        help="Specify path to install executables")
    opt.add_option ('--libdir', default='', type='string', dest='libdir', \
        help="Specify path to install libraries")
    
    opt.add_option ('--test', default=False, action='store_true', dest='test', \
        help="Build the test suite")
    opt.add_option ('--ziptype', default='gz', dest='ziptype', type='string', 
        help='Zip type for waf dist (gz/bz2/zip) [ Default: gz ]')

    opt.add_option ('--minimal', dest='minimal', default=False, action='store_true')

def silence_warnings (conf):
    '''TODO: resolve these'''
    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-declarations', '-Wno-attributes'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-declarations', '-Wno-attributes'])
    
    if 'clang' in conf.env.CXX[0]:
        conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
        conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register'])
        conf.env.append_unique ('CFLAGS', ['-Wno-dynamic-class-memaccess'])
        conf.env.append_unique ('CXXFLAGS', ['-Wno-dynamic-class-memaccess'])

def set_install_dir (conf, key, d1, d2):
    if not isinstance (d1, str):
        d1 = ''
    conf.env[key] = d1.strip()
    if len(conf.env[key]) <= 0: 
        conf.env[key] = d2.strip()

def configure (conf):
    conf.env.TEST       = bool(conf.options.test)
    conf.env.DEBUG      = bool(conf.options.debug)

    conf.load ('host depends')
    conf.load ("compiler_c compiler_cxx")
    conf.check_cxx_version()

    set_install_dir (conf, 'BINDIR', conf.options.bindir,
                     os.path.join (conf.env.PREFIX, 'bin'))
    set_install_dir (conf, 'LIBDIR', conf.options.libdir, 
                     os.path.join (conf.env.PREFIX, 'lib'))
    set_install_dir (conf, 'INCLUDEDIR', None,
                     os.path.join (conf.env.PREFIX, 'include/element'))
                    
    conf.env.DATADIR    = os.path.join (conf.env.PREFIX,  'share/element')
    conf.env.DOCDIR     = os.path.join (conf.env.PREFIX,  'share/doc/element')
    conf.env.VSTDIR     = os.path.join (conf.env.LIBDIR,  'vst')
    conf.env.VST3DIR    = os.path.join (conf.env.LIBDIR,  'vst3')
    
    if 'mingw32' in ' '.join (conf.env.CC) or 'mingw32' in ' '.join (conf.env.CXX):
        conf.env.HOST_PLATFORM = 'win32'

    conf.load ('ccache')
    conf.load ("git")
    conf.load ("juce")
    conf.load ('scripting')
    conf.load ('bundle')
    conf.load ('templates')

    silence_warnings (conf)

    conf.find_projucer (mandatory=False)
    conf.find_program ('convert', mandatory=False)
    
    conf.check_common()
    if conf.env.HOST_PLATFORM == 'win32': conf.check_mingw()
    elif juce.is_mac(): conf.check_mac()
    else: conf.check_linux()

    conf.define ('KV_DOCKING_WINDOWS', 1)
    if len(conf.env.GIT_HASH) > 0:
        conf.define ('EL_GIT_VERSION', conf.env.GIT_HASH)

    conf.define ('JUCE_DLL', True)
    conf.define ('JUCE_STANDALONE_APPLICATION', False)
    conf.define ('JUCE_DISPLAY_SPLASH_SCREEN', False)
    conf.define ('JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED', True)
    for m in element.juce_modules.split():
        conf.define ('JUCE_MODULE_AVAILABLE_%s' % m, True)
    
    conf.write_config_header (configfile='include/element/config.h',
                              guard='EL_CONFIG_H')
    
    print
    juce.display_header ("Element")
    conf.message ("Config", 'Debug' if conf.options.debug else 'Release')
    
    print
    juce.display_header ("Audio Devices")
    if conf.host_is_linux():
        conf.message ("Alsa",       conf.env.ALSA)
    conf.message ("JACK Audio", conf.env.JACK)
    if conf.host_is_windows():
        conf.message ("ASIO",       bool(conf.env.HAVE_ASIO))
    
    print
    juce.display_header ("Plugin Host")
    conf.message ("AudioUnit",  juce.is_mac())
    conf.message ("VST2",       conf.env.VST)
    conf.message ("VST3",       conf.env.VST3)
    conf.message ("LADSPA",     bool(conf.env.LADSPA))
    conf.message ("LV2",        bool(conf.env.LV2))
    print
    # juce.display_header ("Plugin Clients")
    # conf.message ("VST2",       conf.env.VST)
    print
    juce.display_header ("Paths")
    conf.message ("Prefix",      conf.env.PREFIX)
    conf.message ("Programs",    conf.env.BINDIR)
    conf.message ("Libraries",   conf.env.LIBDIR)
    conf.message ("Data",        conf.env.DATADIR)
    conf.message ("Lua",         conf.env.LUADIR)
    conf.message ("Scripts",     conf.env.SCRIPTSDIR)
    if conf.env.VST:
        conf.message ("VST",     conf.env.VSTDIR)
    print
    juce.display_header ("Compiler")
    conf.display_archs()
    conf.message ("CC",             ' '.join (conf.env.CC))
    conf.message ("CXX",            ' '.join (conf.env.CXX))
    conf.message ("CFLAGS",         conf.env.CFLAGS)
    conf.message ("CXXFLAGS",       conf.env.CXXFLAGS)
    conf.message ("LINKFLAGS",      conf.env.LINKFLAGS)

def common_includes():
    return [
        VST3_PATH, \
        'libs/JUCE/modules', \
        'libs/kv/modules', \
        'libs/jlv2/modules', \
        'libs/compat', \
        'libs/element/lua', \
        'build/include', \
        'src'
    ]

def lua_kv_sources (ctx):
    return ctx.path.ant_glob ('libs/lua-kv/src/kv/**/*.c') + \
           ctx.path.ant_glob ('libs/lua-kv/src/kv/**/*.cpp')

def juce_sources (ctx):
    return element.get_juce_library_code ("libs/compat")

def element_sources (ctx):
    lua_mod_sources = '''
        libs/element/lua/el/audio.c
        libs/element/lua/el/bytes.c
        libs/element/lua/el/midi.c
        libs/element/lua/el/round.c
        libs/element/lua/el/vector.c
    '''.split()
    lib_sources = '''
        libs/element/src/bindings.cpp
        libs/element/src/context.cpp
        libs/element/src/module.cpp
        libs/element/src/scripting.cpp
        libs/element/src/strings.cpp
        libs/element/src/video.cpp
        libs/element/src/evg/context.cpp
        libs/element/src/evg/device.cpp
        libs/element/src/evg/image_source.cpp
        libs/element/src/evg/program.cpp
        libs/element/src/evg/solid_source.cpp
        libs/element/src/evg/source.cpp
        '''.split()
    if ctx.host_is_windows():
        lib_sources.append ('libs/element/src/dlfcn-win32.c')
    
    sources = lua_mod_sources + lib_sources
    return sources

def element_juce_sources (ctx):
    sources  = element.kv_module_code (ctx, "libs/compat")
    return sources

def element_juce_app_sources (ctx):
    sources = ctx.path.ant_glob ('src/**/*.cpp')
    sources += ctx.path.ant_glob ('libs/compat/BinaryData*.cpp')
    return sources

def build_desktop (bld, slug='element'):
    if not juce.is_linux():
        return

    if bool(bld.env.CONVERT):
        for size in '16 32 64 128 256 512'.split():
            geometry = '%sx%s' % (size, size)
            bld (
                rule   = '%s ${SRC} -resize %s ${TGT}' % (bld.env.CONVERT[0], geometry),
                source = 'data/ElementIcon.png',
                target = 'share/icons/hicolor/%s/apps/net.kushview.element.png' % geometry,
                install_path = bld.env.PREFIX + '/share/icons/hicolor/%s/apps' % geometry
            )
    else:
        bld (
            rule = 'cp -f ${SRC} ${TGT}',
            source = 'data/ElementIcon_512x512.png',
            target = 'share/icons/hicolor/512x512/apps/net.kushview.element.png',
            install_path = bld.env.PREFIX + '/share/icons/hicolor/512x512/apps'
        )

    src = "data/net.kushview.%s.desktop.in" % (slug)
    tgt = "share/applications/net.kushview.%s.desktop" % (slug)

    if os.path.exists (src):
        bld (features = "subst",
             source         = src,
             target         = tgt,
             name           = tgt,
             ELEMENT_EXE    = 'element',
             ELEMENT_ICON   = 'net.kushview.element',
             install_path   = bld.env.PREFIX + "/share/applications"
        )

    bld.install_files (bld.env.DATADIR, 'data/ElementIcon.png')

def build_lua_docs (bld):
    if bool(bld.env.LDOC):
        call ([bld.env.LDOC[0], '-f', 'markdown', '.' ])

def build_liblua (bld):
    pass
    # luaEnv = bld.env.derive()
    # for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
    #     luaEnv.append_unique (k, [ '-fPIC' ])

    # lua_kv = bld (
    #     name     = 'LUA_KV',
    #     target   = 'lib/lua-kv',
    #     env      = luaEnv,
    #     install_path = None,
    #     features = 'cxx cxxshlib',
    #     includes = common_includes() + [ 'libs/element/include', \
    #                                      'libs/lua-kv/include', \
    #                                      'libs/lua-kv/src' ],
    #     source = lua_kv_sources (bld),
    #     use = [ 'LUA' ],
    #     defines = [ 'JUCE_DLL_BUILD=1' ]
    # )

    # lua_kv.export_includes = lua_kv.includes
    # bld.add_group()

def add_scripts_to (bld, builddir, instdir, 
                    modsdir='Modules', 
                    scriptsdir='Scripts'):
    for node in bld.path.ant_glob ('libs/element/lua/el/*.lua'):
        s = bld (
            features    ='subst', 
            source      = node,
            target      = '%s/%s/el/%s' % (builddir, modsdir, node.name),
            install_path= '%s/%s/el' % (instdir, modsdir) if instdir else None
        )
    for node in bld.path.ant_glob ('libs/lua-kv/src/kv/*.lua'):
        bld (
            features    ='subst',
            source      = node,
            target      = '%s/%s/kv/%s' % (builddir, modsdir, node.name),
            install_path= '%s/%s/kv' % (instdir, modsdir) if instdir else None
        )
    for node in bld.path.ant_glob ('scripts/**/*.lua'):
        bld (
            features    ='subst',
            source      = node,
            target      = '%s/%s/%s' % (builddir, scriptsdir, node.name),
            install_path= '%s/%s' % (instdir, scriptsdir) if instdir else None
        )

def build_vst_linux (bld, plugin):
    vstEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        vstEnv.append_unique (k, [ '-fPIC' ])
    vstEnv.cxxshlib_PATTERN = bld.env.plugin_PATTERN
    vst = bld (
        features        = 'cxx cxxshlib',
        source          = [
            'tools/jucer/%s/Source/%s.cpp' % (plugin, plugin),
            'libs/compat/include_juce_audio_plugin_client_VST2.cpp'
        ],
        includes        = common_includes(),
        target          = 'plugins/VST/%s' % plugin,
        name            = 'ELEMENT_VST',
        env             = vstEnv,
        use             = [ 'ELEMENT', 'LUA', 'LIBJUCE' ],
        install_path    = '%s/Kushview' % bld.env.VSTDIR,
    )

def build_vst (bld):
    if bld.env.VST and bld.host_is_linux():
        for plugin in 'Element ElementFX'.split():
            build_vst_linux (bld, plugin)

def vst3_bundle_arch():
    if juce.is_mac():
        return 'MacOS'
    if juce.is_linux():
        return 'x86_64-linux'
    return ''

def build_vst3_linux (bld, plugin):
    if not bld.env.VST3:
        return

    vstEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        vstEnv.append_unique (k, [ '-fPIC' ])
    
    vstEnv.cxxshlib_PATTERN = bld.env.plugin_PATTERN
    vst3 = bld.bundle (
        name            = '%s_BUNDLE_VST3' % plugin,
        target          = 'plugins/VST3/%s.vst3' % plugin,
        install_path    = '%s/Kushview' % (bld.env.VST3DIR)
    )

    binary = vst3.bundle_create_child ('Contents/%s/%s' % (vst3_bundle_arch(), plugin),
        features        = 'cxx cxxshlib',
        name            = '%s_VST3' % plugin,        
        source          = [
            'tools/jucer/%s/Source/%s.cpp' % (plugin, plugin),
            'libs/compat/include_juce_audio_plugin_client_VST3.cpp'
        ],
        includes        = common_includes(),
        env             = vstEnv,
        use             = [ 'ELEMENT', 'LUA', vst3.name ]
    )

    add_scripts_to (bld,
        '%s/Contents/Resources' % vst3.bundle_node(),
        '%s/Contents/Resources' % vst3.bundle_install_path()
    )

def build_vst3 (bld):
    if juce.is_linux():
        for plugin in 'Element ElementFX'.split():
            build_vst3_linux (bld, plugin)

def build_libjuce (bld):
    libEnv = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        libEnv.append_unique (k, [ '-fPIC' ])

    libjuce = bld (
        features    = 'cxx cxxstlib',
        source      = juce_sources (bld),
        includes    = common_includes(),
        target      = 'lib/juce',
        name        = 'LIBJUCE',
        env         = libEnv,
        use         = [ 'DEPENDS', 'ASIO' ],
        defines     = [],
        cxxflags    = [ '-fvisibility=hidden' ],
        linkflags   = [],
        install_path = None
    )

    if bld.host_is_linux():
        libjuce.use += ['FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT', 'CURL']
    elif bld.host_is_windows():
        libjuce.defines.append ('JUCE_DLL_BUILD=1')
    bld.add_group()

def build_libelement (bld):
    env = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        env.append_unique (k, [ '-fPIC' ])
    
    library = bld (
        features    = 'cxx cxxshlib',
        source      = element_sources (bld),
        includes    = [
            'libs/element/include',
            'build/include'
        ],
        target      = 'lib/element',
        name        = 'ELEMENT',
        env         = env,
        use         = [ 'DEPENDS', 'LUA' ],
        cflags      = [ '-fvisibility=hidden' ],
        cxxflags    = [ '-fvisibility=hidden' ],
        defines     = [ 'EL_PRO=1', 'EL_DLLEXPORT=1' ],
        linkflags   = [],
        vnum        = '0.47.0',
        install_path = bld.env.LIBDIR
    )

    bld (
        features      = 'subst',
        source        = 'element.pc.in',
        target        = 'element.pc',
        NAME          = 'element',
        LIBNAME       = os.path.basename (library.target),
        PREFIX        = bld.env.PREFIX,
        VERSION       = library.vnum,
        INCLUDEDIR    = os.path.join (bld.env.PREFIX, 'include'),
        install_path  = os.path.join (library.install_path, 'pkgconfig')
    )

    if bld.host_is_linux():
        library.use.append ('DL')
        library.use.append ('PTHREAD')
    
    library.export_includes = library.includes
    bld.add_group()

def build_libelement_opengl (bld):
    bld.recurse ('libs/element/opengl')

def build_libelement_juce (bld):
    env = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        env.append_unique (k, [ '-fPIC' ])
    
    jsources = juce_sources (bld)
    
    el_lua_sources = '''
        libs/element/lua/el/AudioBuffer32.cpp
        libs/element/lua/el/AudioBuffer64.cpp
        libs/element/lua/el/MidiMessage.cpp
        libs/element/lua/el/MidiBuffer.cpp
        libs/element/lua/el/Graphics.cpp
        libs/element/lua/el/Point.cpp
        libs/element/lua/el/Range.cpp
        libs/element/lua/el/Rectangle.cpp
        libs/element/lua/el/Bounds.cpp
        libs/element/lua/el/TextButton.cpp
        libs/element/lua/el/Widget.cpp
        libs/element/lua/el/Desktop.cpp
        libs/element/lua/el/DocumentWindow.cpp
        libs/element/lua/el/MouseEvent.cpp
        libs/element/lua/el/File.cpp
        libs/element/lua/el/Slider.cpp
    '''.split()
    
    # NOTE: here for testing purposes
    app_sources = []
    # app_sources = element_juce_app_sources (bld) + \
    #               element_juce_sources (bld)
    
    library = bld (
        features    = 'cxx cxxshlib',
        source      = el_lua_sources + jsources + app_sources,
        includes    = common_includes(),
        target      = 'lib/element-juce',
        name        = 'ELEMENT_JUCE',
        env         = env,
        defines     = [ 'EL_PRO=1' ],
        use         = [ 'DEPENDS', 'ELEMENT', 'LUA' ],
        cflags      = [ '-fvisibility=hidden' ],
        cxxflags    = [ '-fvisibility=hidden', '-Wno-implicit-int-float-conversion' ],
        linkflags   = [ '-fvisibility=hidden' ],
        vnum        = '0.47.0',
        install_path = bld.env.LIBDIR
    )

    if bld.env.JACK:    library.use += [ 'JACK' ]

    if bld.host_is_linux():
        library.use += ['FREETYPE2', 'X11', 'DL', 'PTHREAD', 'ALSA', 'XEXT', 'CURL']
        # library.cxxflags += [
        #     '-DLUA_PATH_DEFAULT="%s"'  % env.LUA_PATH_DEFAULT,
        #     '-DLUA_CPATH_DEFAULT="%s"' % env.LUA_CPATH_DEFAULT,
        #     '-DEL_LUADIR="%s"'         % env.LUADIR,
        #     '-DEL_SCRIPTSDIR="%s"'     % env.SCRIPTSDIR
        # ]

    elif bld.host_is_mac():
        library.use += [
            'ACCELERATE', 'AUDIO_TOOLBOX', 'AUDIO_UNIT', 'CORE_AUDIO', 
            'CORE_AUDIO_KIT', 'COCOA', 'CORE_MIDI', 'IO_KIT', 'QUARTZ_CORE',
            'TEMPLATES'
        ]

    elif bld.host_is_mingw32():
        for l in element.mingw_libs.split():
            library.use.append (l.upper())
        if bld.env.DEBUG:
            library.env.append_unique ('CXXFLAGS', ['-Wa,-mbig-obj'])
        # library.defines.append ('EL_DLLEXPORT=1')
        # library.defines.append ('JUCE_DLL_BUILD=1')
        # library.env.append_unique ('LINKFLAGS_STATIC_GCC', [ '-static-libgcc', '-static-libstdc++',
        #                                                  '-Wl,-Bstatic,--whole-archive', '-lwinpthread', 
        #                                                  '-Wl,--no-whole-archive' ])
        # library.use += [ 'STATIC_GCC' ]

    pcfile = bld (
        features      = 'subst',
        source        = 'element.pc.in',
        target        = 'element-juce.pc',
        NAME          = 'element-juce',
        LIBNAME       = os.path.basename (library.target),
        PREFIX        = bld.env.PREFIX,
        VERSION       = library.vnum,
        REQUIRES_PRIVATE    = 'element >= %s' % library.vnum,
        INCLUDEDIR    = os.path.join (bld.env.PREFIX, 'include'),
        CFLAGS_EXTRA  = '-I${includedir}/element/juce/modules',
        install_path  = os.path.join (library.install_path, 'pkgconfig')
    )

    library.export_includes = library.includes
    bld.add_group()

def build_console_app (bld):
    bld.recurse ('console')

def build_juce_app (bld):
    import plugins
    appEnv = plugins.derive_env (bld)
    
    sources = [ 'UI/UI.cpp' ]
    sources += element_juce_app_sources (bld) + \
               element_juce_sources (bld)

    app = bld (
        features    = 'cxx cxxshlib',
        source      = sources,
        includes    = common_includes(),
        target      = 'modules/UI.element/UI',
        name        = 'ELEMENT_UI',
        env         = appEnv,
        use         = [ 'ELEMENT', 'ELEMENT_JUCE', 'LUA' ],
        cflags      = ['-fvisibility=hidden' ],
        cxxflags    = ['-fvisibility=hidden' ],
        linkflags   = ['-fvisibility=hidden' ],
        defines     = ['EL_PRO=1']
    )
    bld (
        features='subst',
        source ='UI/manifest.lua',
        target = 'modules/UI.element/manifest.lua',
    )
    app.includes.append ('libs/lua')
    app.includes.append ('libs/lua-kv/src')
    app.includes.append ('libs/lua-kv/include')

    if bld.host_is_linux():
        build_desktop (bld)

    # elif bld.host_is_mac():
    #     app.target       = 'Applications/Element'
    #     app.mac_app      = True
    #     app.mac_plist    = 'build/data/Info.plist'
    #     app.mac_files    = [ 'data/Icon.icns' ]
    #     add_scripts_to (bld, '%s.app/Contents/Resources' % app.target, None)

    # elif bld.host_is_mingw32():
    #     # app.env.append_unique ('LINKFLAGS_STATIC_GCC', [ '-static-libgcc', '-static-libstdc++',
    #     #                                                  '-Wl,-Bstatic,--whole-archive', '-lwinpthread', 
    #     #                                                  '-Wl,--no-whole-archive' ])
    #     # app.use += [ 'STATIC_GCC' ]
    #     app.install_path = bld.env.BINDIR

def install_lua_files (bld):
    if not bld.host_is_linux() and not bld.host_is_mingw32():
        return
    path = bld.path
    join = os.path.join
    bld.install_files (bld.env.SCRIPTSDIR,
                       path.ant_glob ("scripts/**/*.lua"),
                       relative_trick=True,
                       cwd=path.find_dir ('scripts'))

    bld.install_files (join (bld.env.DOCDIR, 'lua'),
                       bld.path.ant_glob ("build/doc/lua/**/*.*", quiet=True),
                       relative_trick=True,
                       cwd=path.find_dir ('build/doc/lua'))

    bld.install_files (bld.env.LUADIR,
                       bld.path.ant_glob ("libs/lua-kv/src/**/*.lua"),
                       relative_trick=True,
                       cwd=path.find_dir ('libs/lua-kv/src'))

    bld.install_files (bld.env.LUADIR,
                       path.ant_glob ("libs/element/lua/**/*.lua"),
                       relative_trick=True,
                       cwd=path.find_dir ('libs/element/lua'))

def build (bld):
    if bld.is_install:
        if juce.is_mac(): bld.fatal ("waf install not supported on OSX")
        if bld.options.minimal: return

    if bld.cmd == 'build' and git.is_repo():
        bld.add_pre_fun (bld.git_update_env)

    bld.template (
        name = 'TEMPLATES',
        source = bld.path.ant_glob ('src/**/*.h.in') \
               + bld.path.ant_glob ('data/**/*.in') \
               + bld.path.ant_glob ("tools/**/*.in"),
        install_path = None,
        PACKAGE_VERSION = VERSION
    )

    bld.add_group()
    if bld.options.minimal:
        return

    build_liblua (bld)
    build_libelement_opengl (bld)
    build_libelement (bld)
    build_libelement_juce (bld)
    
    bld.add_group()
    build_juce_app (bld)
    bld.add_group()
    build_console_app (bld)

    install_lua_files (bld)

    modstobuild = 'JLV2'.split()
    for mod in modstobuild:
        bld.recurse ('modules/%s.element' % mod)
    
    # build_vst (bld)
    # build_vst3 (bld)

    if bld.env.TEST:
        bld.recurse ('test')

    bld.install_files (os.path.join (bld.env.PREFIX, 'include/element'),
                       bld.path.ant_glob ("libs/element/include/element/**/*.h") + \
                       bld.path.ant_glob ("libs/element/include/element/**/*.hpp"),
                       relative_trick=True, cwd=bld.path.find_dir ('libs/element/include/element'))
    
    bld.install_files (os.path.join (bld.env.PREFIX, 'include/element/juce/modules'), \
        bld.path.ant_glob ("libs/JUCE/modules/**/*.h"), \
        relative_trick=True, cwd=bld.path.find_dir ('libs/JUCE/modules'))

def check (ctx):
    if not os.path.exists('build/bin/test_juce'):
        ctx.fatal ("JUCE tests not compiled")
        return
    if not os.path.exists('build/bin/test_element'):
        ctx.fatal ("Test suite not compiled")
        return
    os.environ["LD_LIBRARY_PATH"] = "build/lib"
    failed = 0
    print ("JUCE Tests")
    if 0 != call (["build/bin/test_juce"]):
        failed += 1
    print ("Done!\n")

    print ("Element Test Suite")
    if 0 != call (["build/bin/test_element"]):
        failed += 1
    if (failed > 0):
        ctx.fatal ("Test suite exited with fails")

def dist (ctx):
    z = ctx.options.ziptype
    if 'zip' in z:
        ziptype = z
    else:
        ziptype = "tar." + z

    ctx.base_name = '%s-%s' % (APPNAME, VERSION)
    ctx.algo = ziptype
    ctx.excl = ' **/.waf-1* **/.waf-2* **/.waf3-* **/*~ **/*.pyc **/*.swp **/.lock-w*'
    ctx.excl += ' **/.gitignore **/.gitmodules **/.git dist deploy element* pro **/Builds **/build'
    ctx.excl += ' **/.DS_Store **/.vscode **/.travis.yml *.bz2 *.zip *.gz'
    ctx.excl += ' tools/jucer/**/JuceLibraryCode'

def docs (ctx):
    ctx.add_pre_fun (build_lua_docs)

def version (ctx):
    print (element.VERSION)
    exit(0)

def versionbump (ctx):
    import projects
    ctx.add_pre_fun (projects.update_version)

def resave (ctx):
    import projects
    ctx.add_pre_fun (projects.resave)

def copydlls (ctx):
    import depends
    if ctx.host_is_windows():
        depends.copydlls (ctx)

def clean_artifacts (ctx):
    from subprocess import call
    root = os.path.abspath (os.path.join (os.getcwd(), 'build'))
    if os.path.exists (root):
        call ('bash tools/clean_artifacts.sh'.split())

def deepclean (ctx):
    from waflib import Options
    lst = ['clean_artifacts', 'distclean']
    Options.commands = lst + Options.commands

def cleanbuild (ctx):
    from waflib import Options
    lst = [ 'clean', 'build' ]
    Options.commands = lst + Options.commands

def relink (ctx):
    from waflib import Options
    lst = [ 'clean_artifacts', 'build', 'copydlls' ]
    Options.commands = lst + Options.commands

from waflib.Build import BuildContext

class ResaveBuildContext (BuildContext):
    cmd  = 'resave'
    fun  = 'resave'

class DocsBuildContext (BuildContext):
    cmd = 'docs'
    fun = 'docs'

class VersionBumpContext (BuildContext):
    cmd = 'versionbump'
    fun = 'versionbump'

class CopyDLLsContext (BuildContext):
    cmd = 'copydlls'
    fun = 'copydlls'
