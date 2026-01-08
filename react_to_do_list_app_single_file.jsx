import React, { useEffect, useMemo, useRef, useState } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Checkbox } from "@/components/ui/checkbox";
import { Badge } from "@/components/ui/badge";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import {
  Trash2,
  Plus,
  CheckSquare,
  Square,
  Filter as FilterIcon,
  Search,
  Pencil,
  Check,
  X,
} from "lucide-react";

// Single-file, modern, and keyboard-accessible React To-Do app
// Features: add, edit inline, toggle, delete, search, filter, clear completed, select all, localStorage persistence

const uid = () => (crypto?.randomUUID ? crypto.randomUUID() : `${Date.now()}-${Math.random()}`);

const FILTERS = [
  { key: "all", label: "All" },
  { key: "active", label: "Active" },
  { key: "completed", label: "Completed" },
];

function useLocalStorage(key, initial) {
  const [state, setState] = useState(() => {
    try {
      const raw = localStorage.getItem(key);
      return raw ? JSON.parse(raw) : initial;
    } catch {
      return initial;
    }
  });
  useEffect(() => {
    try {
      localStorage.setItem(key, JSON.stringify(state));
    } catch {}
  }, [key, state]);
  return [state, setState];
}

export default function TodoApp() {
  const [items, setItems] = useLocalStorage("todo.items.v1", []);
  const [text, setText] = useState("");
  const [filter, setFilter] = useLocalStorage("todo.filter.v1", "all");
  const [query, setQuery] = useState("");
  const [editingId, setEditingId] = useState(null);
  const [editingText, setEditingText] = useState("");
  const inputRef = useRef(null);

  const remaining = useMemo(() => items.filter((i) => !i.completed).length, [items]);
  const hasCompleted = useMemo(() => items.some((i) => i.completed), [items]);

  const filtered = useMemo(() => {
    let base = items;
    if (filter === "active") base = base.filter((i) => !i.completed);
    if (filter === "completed") base = base.filter((i) => i.completed);
    if (query.trim()) {
      const q = query.toLowerCase();
      base = base.filter((i) => i.text.toLowerCase().includes(q));
    }
    return base;
  }, [items, filter, query]);

  function addItem() {
    const t = text.trim();
    if (!t) return;
    const newItem = { id: uid(), text: t, completed: false, createdAt: Date.now() };
    setItems([newItem, ...items]);
    setText("");
    inputRef.current?.focus();
  }

  function toggleItem(id) {
    setItems((prev) => prev.map((i) => (i.id === id ? { ...i, completed: !i.completed } : i)));
  }

  function removeItem(id) {
    setItems((prev) => prev.filter((i) => i.id !== id));
    if (editingId === id) {
      setEditingId(null);
      setEditingText("");
    }
  }

  function startEdit(item) {
    setEditingId(item.id);
    setEditingText(item.text);
  }

  function commitEdit() {
    const t = editingText.trim();
    if (!editingId) return;
    if (!t) {
      // empty edit deletes
      removeItem(editingId);
      return;
    }
    setItems((prev) => prev.map((i) => (i.id === editingId ? { ...i, text: t } : i)));
    setEditingId(null);
    setEditingText("");
  }

  function cancelEdit() {
    setEditingId(null);
    setEditingText("");
  }

  function clearCompleted() {
    setItems((prev) => prev.filter((i) => !i.completed));
  }

  function toggleAll() {
    const allDone = items.every((i) => i.completed);
    setItems((prev) => prev.map((i) => ({ ...i, completed: !allDone })));
  }

  function onKeyDownForm(e) {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      addItem();
    }
  }

  return (
    <div className="min-h-screen w-full bg-gradient-to-b from-slate-50 to-white flex items-start justify-center p-6">
      <Card className="w-full max-w-2xl shadow-xl border-slate-200">
        <CardHeader className="space-y-4">
          <div className="flex items-center gap-3">
            <CheckSquare className="h-7 w-7" />
            <CardTitle className="text-2xl">To‑Do List</CardTitle>
            <Badge variant="secondary" className="ml-auto text-xs">
              {remaining} remaining
            </Badge>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
            <div className="md:col-span-2 flex items-center gap-2">
              <Input
                ref={inputRef}
                value={text}
                onChange={(e) => setText(e.target.value)}
                onKeyDown={onKeyDownForm}
                placeholder="Add a new task and press Enter…"
                className="h-11"
                aria-label="New task"
              />
              <Button onClick={addItem} className="h-11 px-5" aria-label="Add task">
                <Plus className="h-4 w-4 mr-1" /> Add
              </Button>
            </div>

            <div className="flex items-center gap-2">
              <div className="relative flex-1">
                <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4" />
                <Input
                  value={query}
                  onChange={(e) => setQuery(e.target.value)}
                  placeholder="Search…"
                  className="pl-9 h-11"
                  aria-label="Search tasks"
                />
              </div>

              <DropdownMenu>
                <DropdownMenuTrigger asChild>
                  <Button variant="outline" className="h-11">
                    <FilterIcon className="h-4 w-4 mr-2" />
                    {FILTERS.find((f) => f.key === filter)?.label}
                  </Button>
                </DropdownMenuTrigger>
                <DropdownMenuContent align="end">
                  {FILTERS.map((f) => (
                    <DropdownMenuItem key={f.key} onClick={() => setFilter(f.key)}>
                      {f.label}
                    </DropdownMenuItem>
                  ))}
                </DropdownMenuContent>
              </DropdownMenu>
            </div>
          </div>

          <div className="flex items-center gap-2 text-sm text-slate-500">
            <Button variant="ghost" size="sm" onClick={toggleAll} aria-label="Toggle all">
              {items.every((i) => i.completed) ? (
                <CheckSquare className="h-4 w-4 mr-2" />
              ) : (
                <Square className="h-4 w-4 mr-2" />
              )}
              Toggle all
            </Button>
            <span className="mx-2">•</span>
            <Button variant="ghost" size="sm" onClick={clearCompleted} disabled={!hasCompleted}>
              Clear completed
            </Button>
          </div>
        </CardHeader>

        <CardContent>
          {filtered.length === 0 ? (
            <div className="text-center text-slate-500 py-10">
              No tasks here yet. Add one above!
            </div>
          ) : (
            <ul className="space-y-2" aria-live="polite">
              <AnimatePresence initial={false}>
                {filtered.map((item) => (
                  <motion.li
                    key={item.id}
                    initial={{ opacity: 0, y: 8 }}
                    animate={{ opacity: 1, y: 0 }}
                    exit={{ opacity: 0, y: -8 }}
                    transition={{ duration: 0.18 }}
                  >
                    <div className="group flex items-center gap-3 rounded-2xl border bg-white px-3 py-2 shadow-sm hover:shadow-md transition-shadow">
                      <Checkbox
                        checked={item.completed}
                        onCheckedChange={() => toggleItem(item.id)}
                        aria-label={item.completed ? "Mark as active" : "Mark as completed"}
                      />

                      {editingId === item.id ? (
                        <form
                          onSubmit={(e) => {
                            e.preventDefault();
                            commitEdit();
                          }}
                          className="flex-1 flex items-center gap-2"
                        >
                          <Input
                            value={editingText}
                            onChange={(e) => setEditingText(e.target.value)}
                            autoFocus
                            onKeyDown={(e) => {
                              if (e.key === "Escape") {
                                e.preventDefault();
                                cancelEdit();
                              }
                            }}
                            aria-label="Edit task"
                          />
                          <Button type="submit" size="sm" className="px-3" aria-label="Save edit">
                            <Check className="h-4 w-4" />
                          </Button>
                          <Button type="button" variant="ghost" size="sm" onClick={cancelEdit} aria-label="Cancel edit">
                            <X className="h-4 w-4" />
                          </Button>
                        </form>
                      ) : (
                        <button
                          className={`flex-1 text-left truncate ${
                            item.completed ? "line-through text-slate-400" : ""
                          }`}
                          onClick={() => startEdit(item)}
                          title="Click to edit"
                        >
                          {item.text}
                        </button>
                      )}

                      <div className="opacity-0 group-hover:opacity-100 transition-opacity flex items-center gap-1">
                        <Button
                          variant="ghost"
                          size="icon"
                          onClick={() => startEdit(item)}
                          aria-label="Edit task"
                        >
                          <Pencil className="h-4 w-4" />
                        </Button>
                        <Button
                          variant="ghost"
                          size="icon"
                          onClick={() => removeItem(item.id)}
                          aria-label="Delete task"
                        >
                          <Trash2 className="h-4 w-4" />
                        </Button>
                      </div>
                    </div>
                  </motion.li>
                ))}
              </AnimatePresence>
            </ul>
          )}
        </CardContent>
      </Card>
    </div>
  );
}
