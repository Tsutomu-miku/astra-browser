import { useRef, type CSSProperties, type ReactNode } from "react";

import { handleMenuKeyboardNavigation, useMenuInitialFocus } from "../../../../common/context-menu/menuKeyboardNavigation";

export function SidebarMenuSurface({
  children,
  className,
  style
}: {
  children: ReactNode;
  className?: string;
  style?: CSSProperties;
}) {
  const menuRef = useRef<HTMLDivElement | null>(null);
  useMenuInitialFocus(menuRef);

  return (
    <div
      ref={menuRef}
      className={["sidebar-menu-surface", className].filter(Boolean).join(" ")}
      role="menu"
      style={style}
      onClick={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.preventDefault()}
      onKeyDown={handleMenuKeyboardNavigation}
    >
      {children}
    </div>
  );
}
