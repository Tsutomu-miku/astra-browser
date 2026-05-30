import { useEffect, useRef } from "react";

export interface ContextMenuCloseOptions {
  restoreFocus?: boolean;
}

export type ContextMenuCloseHandler = (options?: ContextMenuCloseOptions) => void;

export function useContextMenuDismissal({
  isOpen,
  onClose
}: {
  isOpen: boolean;
  onClose: ContextMenuCloseHandler;
}) {
  const onCloseRef = useRef(onClose);

  useEffect(() => {
    onCloseRef.current = onClose;
  }, [onClose]);

  useEffect(() => {
    if (!isOpen) return;

    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === "Escape") onCloseRef.current();
    };
    const closeWithoutFocusRestore = () => onCloseRef.current({ restoreFocus: false });

    window.addEventListener("click", closeWithoutFocusRestore);
    window.addEventListener("blur", closeWithoutFocusRestore);
    window.addEventListener("keydown", closeOnEscape);
    window.addEventListener("scroll", closeWithoutFocusRestore, true);
    return () => {
      window.removeEventListener("click", closeWithoutFocusRestore);
      window.removeEventListener("blur", closeWithoutFocusRestore);
      window.removeEventListener("keydown", closeOnEscape);
      window.removeEventListener("scroll", closeWithoutFocusRestore, true);
    };
  }, [isOpen]);
}
