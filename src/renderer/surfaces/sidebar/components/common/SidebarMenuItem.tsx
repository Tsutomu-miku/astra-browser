import type { ButtonHTMLAttributes, ReactNode } from "react";
import type { IconType } from "react-icons";

interface SidebarMenuItemProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  children: ReactNode;
  icon: IconType;
}

export function SidebarMenuItem({
  children,
  className,
  icon: Icon,
  type = "button",
  ...props
}: SidebarMenuItemProps) {
  return (
    <button
      className={["sidebar-menu-item", className].filter(Boolean).join(" ")}
      type={type}
      {...props}
    >
      <span className="sidebar-menu-item-icon" aria-hidden="true"><Icon /></span>
      <span className="sidebar-menu-item-label">{children}</span>
    </button>
  );
}

export function SidebarMenuSeparator() {
  return <span className="sidebar-menu-separator" />;
}
