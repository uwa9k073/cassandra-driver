// SPDX-License-Identifier: Apache-2.0
/**

userver Cassandra Driver
https://github.com/uwa9k073/cassandra-driver

A toolbar button for doxygen-awesome-css that links to the project's
GitHub repository. Drop-in companion to doxygen-awesome-darkmode-toggle.js:
appends itself to the same MSearchBox toolbar so the button appears right
next to the dark-mode toggle.

Usage — add to header.html after the other doxygen-awesome <script> tags:

    <script type="text/javascript"
            src="$relpath^doxygen-awesome-github-link.js"></script>

Then call inside the existing init block:

    DoxygenAwesomeGithubLink.init();

*/

class DoxygenAwesomeGithubLink {

    // GitHub Invertocat SVG (https://github.com/logos — MIT/free-use)
    static icon = `<svg xmlns="http://www.w3.org/2000/svg"
            viewBox="0 0 98 96" width="22" height="22" aria-hidden="true">
        <path fill-rule="evenodd" clip-rule="evenodd"
            d="M48.854 0C21.839 0 0 22 0 49.217c0 21.756 13.993 40.172
               33.405 46.69 2.427.49 3.316-1.059 3.316-2.362
               0-1.141-.08-5.052-.08-9.127-13.59 2.934-16.42-5.867-16.42-5.867
               -2.184-5.704-5.42-7.17-5.42-7.17-4.448-3.015.324-3.015.324-3.015
               4.934.326 7.523 5.052 7.523 5.052 4.367 7.496 11.404 5.378
               14.235 4.074.404-3.178 1.699-5.378 3.074-6.6
               -10.839-1.141-22.243-5.378-22.243-24.283 0-5.378 1.94-9.778
               5.014-13.2-.485-1.222-2.184-6.275.486-13.038 0 0
               4.125-1.304 13.426 5.052a46.97 46.97 0 0 1 12.214-1.63
               c4.125 0 8.33.571 12.213 1.63 9.302-6.356 13.427-5.052
               13.427-5.052 2.67 6.763.97 11.816.485 13.038
               3.155 3.422 5.015 7.822 5.015 13.2 0 18.905-11.404
               23.06-22.324 24.283 1.78 1.548 3.316 4.481 3.316 9.126
               0 6.6-.08 11.897-.08 13.526 0 1.304.89 2.853 3.316 2.364
               19.412-6.52 33.405-24.935 33.405-46.691C97.707 22 75.788 0
               48.854 0z"
            fill="currentColor"/>
    </svg>`

    // Repository URL shown in the button tooltip and as the href
    static url   = "https://github.com/uwa9k073/cassandra-driver"
    static title = "View source on GitHub"

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * Initialise the button and insert it into the doxygen-awesome toolbar.
     *
     * Must be called after the DOM is ready (the same way the other
     * DoxygenAwesome* helpers are called in header.html).
     */
    static init() {
        $(function() {
            $(document).ready(function() {
                DoxygenAwesomeGithubLink._attach()
            })
            // Re-attach on resize because doxygen re-renders the toolbar
            $(window).resize(function() {
                DoxygenAwesomeGithubLink._attach()
            })
        })
    }

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * Build the anchor element once and return it. Subsequent calls return
     * the same cached element so we never create duplicate buttons.
     */
    static _buildElement() {
        if (DoxygenAwesomeGithubLink._element) {
            return DoxygenAwesomeGithubLink._element
        }

        const a = document.createElement('a')
        a.id        = "doxygen-awesome-github-link"
        a.href      = DoxygenAwesomeGithubLink.url
        a.target    = "_blank"
        a.rel       = "noopener noreferrer"
        a.title     = DoxygenAwesomeGithubLink.title
        a.setAttribute("aria-label", DoxygenAwesomeGithubLink.title)
        a.innerHTML = DoxygenAwesomeGithubLink.icon

        // Inherit foreground colour from the active doxygen-awesome theme so
        // the icon automatically adapts to both light and dark modes.
        a.style.cssText = [
            "display:         inline-flex",
            "align-items:     center",
            "justify-content: center",
            "width:           34px",
            "height:          34px",
            "margin:          0 2px",
            "border-radius:   4px",
            "color:           var(--page-foreground-color)",
            "opacity:         0.7",
            "transition:      opacity .15s ease, background-color .15s ease",
            "text-decoration: none",
        ].join(";")

        a.addEventListener("mouseover", () => {
            a.style.opacity         = "1"
            a.style.backgroundColor = "var(--separator-color)"
        })
        a.addEventListener("mouseout", () => {
            a.style.opacity         = "0.7"
            a.style.backgroundColor = "transparent"
        })

        DoxygenAwesomeGithubLink._element = a
        return a
    }

    /** Insert (or re-insert after a resize) the button into the toolbar. */
    static _attach() {
        const searchBox = document.getElementById("MSearchBox")
        if (!searchBox) return

        const toolbar = searchBox.parentNode
        if (!toolbar) return

        const btn = DoxygenAwesomeGithubLink._buildElement()

        // Avoid double-insertion: only append if not already a child
        if (!toolbar.contains(btn)) {
            // Insert before the search box so the button order is:
            //   [GitHub] [dark-mode toggle] [search]
            // Adjust to taste by swapping insertBefore -> appendChild.
            toolbar.insertBefore(btn, searchBox)
        }
    }

    /** @type {HTMLAnchorElement | null} cached DOM element */
    static _element = null
}
