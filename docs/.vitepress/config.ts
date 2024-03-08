import { defineConfig, type DefaultTheme } from 'vitepress'

const commit = await (await fetch("https://api.github.com/repos/gottyduke/dkutil/commits/master")).json()
const lastUpdate = new Date(commit.commit.author.date).toISOString().slice(0, 10)

export default defineConfig({
    base: '/DKUtil/',

    title: 'DKUtil',
    description: 'Some utility headers to help with windows x64 native plugin development',
    lang: 'en-US',

    lastUpdated: true,
    cleanUrls: true,
    metaChunk: true,

    themeConfig: {
        socialLinks: [
            { icon: 'github', link: 'https://github.com/gottyduke' }
        ],

        search: {
            provider: 'local',
        },

        nav: makeNavBar(),

        sidebar: {
            '/dkutil/': { base: '/dkutil/', items: makeSidebarDKUtil() },
            '/logger/': { base: '/logger/', items: makeSidebarLogger() },
            '/config/': { base: '/config/', items: makeSidebarConfig() },
            '/hooks/': { base: '/hooks/', items: makeSidebarHooks() },
            '/utils/': { base: '/utils/', items: makeSidebarUtils() },
            '/extra/': { base: '/extra/', items: makeSidebarExtra() },
        },

        editLink: {
            pattern: 'https://github.com/gottyduke/dkutil/edit/master/docs/:path',
            text: 'Edit current page on GitHub'
        },

        footer: {
            message: 'Released under the MIT License',
            copyright: 'Copyright © 2020-present DK'
        },
    }
})

function makeNavBar(): DefaultTheme.NavItem[] {
    return [
        {
            text: `DKUtil(${lastUpdate})`,
            items: [
                {
                    text: 'About this Project',
                    link: '/dkutil/about',
                    activeMatch: '/dkutil/'
                },
                {
                    text: 'Projects with DKUtil',
                    link: '/dkutil/references',
                    activeMatch: '/dkutil/'
                },
            ]
        },
        {
            text: 'Logger',
            link: '/logger/setup',
            activeMatch: '/logger/'
        },
        {
            text: 'Config',
            link: '/config/config-workflow',
            activeMatch: '/config/'
        },
        {
            text: 'Hook',
            link: '/hooks/hooking-with-dkutil',
            activeMatch: '/hooks/'
        },
        {
            text: 'Utils',
            link: '/utils/templates',
            activeMatch: '/utils/'
        },
        {
            text: 'Extra',
            link: '/extra/skse',
            activeMatch: '/extra/'
        },
    ]
}

function makeSidebarDKUtil(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'DKUtil',
            collapsed: false,
            items: [
                { text: 'About this Project', link: 'about' },
                { text: 'Compilation Flags', link: 'compilation' },
                { text: 'Projects with DKUtil', link: 'references' },
            ]
        },
    ]
}

function makeSidebarLogger(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Logger',
            collapsed: false,
            items: [
                { text: 'Setup', link: 'setup' },
                { text: 'Macros', link: 'macros' },
            ]
        },
    ]
}

function makeSidebarConfig(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Config',
            collapsed: false,
            items: [
                { text: 'Configure How-to', link: 'config-workflow' },
                { text: 'Data Types', link: 'data-types' },
                { text: 'Setup Proxy', link: 'setup-proxy' },
                { text: 'Schema Parser', link: 'schema-whatnot' },
                { text: 'File Helper', link: 'file-helpers' },
            ]
        },
        {
            text: 'Examples',
            collapsed: false,
            items: [
                { text: 'Singleton Settings', link: 'examples' },
                { text: 'External References', link: 'references' },
            ]
        },
    ]
}

function makeSidebarHooks(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Hooks & Memory',
            collapsed: false,
            items: [
                { text: 'Hooking with DKUtil', link: 'hooking-with-dkutil' },
                { text: 'Memory Edit', link: 'memory-edit' },
                { text: 'Address Fetching', link: 'address-fetching' },
                { text: 'Hook Handles', link: 'hook-handles' },
                { text: 'Trampoline', link: 'trampoline' },
            ]
        },
        {
            text: 'API',
            collapsed: false,
            items: [
                { text: 'Relocation', link: 'relocation' },
                { text: 'ASM Patch', link: 'asm-patch' },
                { text: 'Cave Hook', link: 'cave-hook' },
                { text: 'VTable Swap', link: 'vtable-swap' },
                { text: 'IAT Swap', link: 'iat-swap' },
                { text: 'LTO-Enabled Hook', link: 'lto-enabled-hook' },
            ]
        },
        {
            text: 'Examples',
            collapsed: false,
            items: [
                { text: 'Useful Helpers', link: 'useful-helpers' },
                { text: 'Callsite Logging', link: 'callsite-logging' },
                { text: 'External References', link: 'references' },
            ]
        },
    ]
}

function makeSidebarUtils(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Utils',
            collapsed: false,
            items: [
                { text: 'Templates', link: 'templates' },
                { text: 'Enumeration', link: 'enumeration' },
                { text: 'String', link: 'string' },
                { text: 'Numbers', link: 'numbers' },
            ]
        },
    ]
}

function makeSidebarExtra(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Extra',
            collapsed: false,
            items: [
                { text: 'Serializable', link: 'skse' },
            ]
        },
    ]
}