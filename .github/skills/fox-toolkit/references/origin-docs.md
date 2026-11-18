# FOX 1.7.95 origin documentation index

`fox-1.7.95/doc/fox.html` is a frameset, not a content page. Its navigation
starts with `menu.html`, while the useful programming guides are standalone
HTML files in the same directory. Use these files as version-specific design
references, then confirm exact signatures and current behavior in the origin
headers/implementation and the matching `fox2/` files.

## Concept and lifecycle guides

| Topic | Origin guide | What to extract |
|---|---|---|
| First FOX application | `introduction.html` | Scribble example, startup order, custom drawing and basic messages |
| Application/event loop | `app.html` | `FXApp` lifetime, `init()`, `create()`, `run()`, delayed repaint and event-queue cooperation |
| Message dispatch | `messages.html` | targets, selectors, message maps, bubbling/return-value expectations |
| Widget authoring | `widgets.html` | subclass selection, object construction, default-size negotiation, `create()` and paint handler |
| Layout | `layout.html` | packing geometry, layout hints and nested managers |
| Window tree | `window.html` | parent/root/shell/owner navigation and child traversal |
| GUI updating | `guiupdate.html` | `SEL_UPDATE` pull pattern for enabled, checked and displayed state |
| Data targets | `datatarget.html` | bidirectional `FXDataTarget` binding and option values |
| Timers and asynchronous work | `timers.html` | one-shot timeouts, absolute deadlines, chores, signals and input notifications |
| Focus and keyboard | `focus.html` | internal focus chain, navigation, `canFocus()` and focus messages |
| Drag and drop | `draganddrop.html` | drag types, source/target roles, DND message sequence and cursor feedback |
| Clipboard and primary selection | `clipboard.html` | ownership, request/reply exchange, clipboard versus primary selection |
| Images and icons | `icons.html` | `FXImage`/`FXIcon`, client pixels, platform resources, image formats and `reswrap` |
| Fonts | `fonts.html` | face/size/weight/slant matching, substitutions and fallback behavior |
| Registry | `registry.html` | desktop/vendor/application settings and system/user precedence |
| Serialization | `serialization.html` | `FXStream`, machine-independent representation and byte order |

## Application and resource lifecycle

1. Construct the one `FXApp`; prefer stack lifetime for the application itself.
2. Call `application.init(argc,argv)` to open the display. The application consumes
   toolkit options and leaves application-specific arguments alone.
3. Construct the top-level window on the heap and attach its widget tree.
4. Call `application.create()` once to realize the tree and create platform-side
   resources recursively.
5. Show the top-level window and enter `application.run()`.

FOX applies the same two-stage principle to widgets and drawable resources.
`FXImage`, `FXIcon`, `FXFont`, and `FXCursor` first hold client-side descriptions
or pixels and then create platform resources with `create()`. A widget normally
creates its attached resources when the widget itself is created. An icon or
image can be shared and created more than once. Use `IMAGE_KEEP` when client
pixels must survive creation for later mutation; otherwise the implementation
may release them. Call `render()` again after changing retained pixels.

If a widget is attached to an already-created application or resource after
startup, explicitly create/destroy it according to the surrounding FOX type's
contract; do not assume C++ construction alone creates a native resource.

## Event loop and delayed work

`FXApp` coordinates native input, repaint requests, timers, chores, signals,
input sources, selection ownership, and drag/drop. It deliberately coalesces
delayed repaint work and performs it after more urgent events, so a sequence of
state changes need not cause one draw per change.

- Timers use nanoseconds and `SEL_TIMEOUT`; each timer is identified by the
  target/selector pair. A timeout is one-shot even if the handler returns `1`.
- Repeating a timer with `addTimeout()` introduces relative-period jitter.
  `addDeadline()` takes an absolute `FXTime`; it is useful for long-lived
  periodic work, but advance to a future deadline so overdue callbacks cannot
  monopolize the loop.
- A chore (`SEL_CHORE`) runs during idle/event-stream exhaustion. It is for
  bounded background work, not an excuse to block the GUI loop.
- Use `hasTimeout()`, `remainingTimeout()`, and matching removal calls in
  destructors. A removal is safe after the callback has already fired.
- Use FOX's signal and input registration APIs when their events must enter the
  GUI message path; still keep handlers short.
- For long computations, periodically service/poll the event loop where the
  surrounding design calls for it, rather than blocking input indefinitely.

## Focus and keyboard navigation

Window-system focus attaches to a top-level window. Inside it, FOX maintains a
focus chain from shell through composites to one leaf control. `setFocus()`
builds that chain upward; `killFocus()` removes a branch. Focus changes can emit
`SEL_FOCUSIN` and `SEL_FOCUSOUT` along multiple ancestors, so use the messages
for visual state rather than assuming only the leaf receives them.

Keyboard events are offered down the focused-child chain. A composite then
interprets navigation keys as `SEL_FOCUS_NEXT`, `SEL_FOCUS_PREV`, left/right and
up/down messages. A custom focusable widget should return true from `canFocus()`
and handle focus/keyboard messages. Check actual focus implementation and test
applications because comments in the historical guide include development-era
notes.

## Clipboard, selection and drag/drop

Treat these as related but distinct transports:

- The clipboard is acquired for explicit cut/copy exchange and survives until
  ownership is lost. The owner advertises formats, handles request messages,
  and releases retained data on loss.
- The primary selection follows a live selection and is commonly not copied out
  of the originating widget.
- Drag and drop is a temporary operation with a drag source, a drop target and
  an action such as reject/copy/move/link.

All three exchange typed data through registered `FXDragType` names. Prefer a
shared type constant and define representation/ownership at the request reply
boundary.

For drop targets, call `dropEnable()`, inspect advertised/requested types, and
handle `SEL_DND_ENTER`, `SEL_DND_MOTION`, `SEL_DND_DROP`, and `SEL_DND_LEAVE`.
Accept or reject during the live operation with `acceptDrop()`. A target must
clean up requested data and temporary visual feedback.

For drag sources, advertise formats with `beginDrag()`, drive `handleDrag()` on
pointer movement, set a cursor based on `didAccept()`, then finish with
`endDrag()`. Serve `SEL_DND_REQUEST` with `setDNDData()` and the correct origin.
For clipboard ownership, use `acquireClipboard()`, then handle gained/lost and
request messages; for primary selection use the corresponding selection APIs
and `FROM_SELECTION` origin.

## Registry and persistent settings

FOX settings are layered by scope: desktop, vendor, and application, each in a
system-wide and a per-user location. Load from general to specific; a more
specific setting shadows a general one. Changes are normally written as
per-user, per-application settings rather than rewriting system defaults.

The historical guide names Unix locations such as `/etc/foxrc` and
`$HOME/.foxrc`. On other platforms, confirm actual `FXSettings`/`FXRegistry`
path handling in the current target implementation. Application and vendor
names passed to `FXApp` participate in this namespace, so changing them changes
where settings are found. Typed reads must still be validated because 1.7.95
has a known zero-string parsing bug documented in the main skill.

## Custom-widget checklist

- Subclass the closest existing widget; inherit border/margin/layout behavior
  instead of duplicating it.
- Add `FXDECLARE` and an `FXIMPLEMENT` plus a message map; allocate unused option
  bits without colliding with base-class flags.
- Put invariants and ordinary defaults in the constructor. Allocate or realize
  platform-dependent resources during `create()` and call base `create()` in
  the expected order.
- Compute useful minimum dimensions in `getDefaultWidth()` and
  `getDefaultHeight()` from content, borders, padding, and current font.
- Implement `layout()` only if the base composite behavior cannot express the
  arrangement.
- Handle `SEL_PAINT` and use the dirty rectangle in `FXEvent`; call `update()`
  for eventual repaint rather than drawing immediately during a command.
- Add focusability, data binding/updates, drag/drop, clipboard ownership or
  timers only when the widget contract needs them; undo callbacks in the
  destructor using the target/ID pair.
- Test with a real layout manager, resize/font/settings variation, and the
  focused platform rather than only at the requested initial coordinates.
