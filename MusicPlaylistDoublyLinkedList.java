class Song {
    String title;
    Song next;
    Song prev;

    public Song(String title) {
        this.title = title;
    }
}

class Playlist {
    private Song head = null;
    private Song current = null;

    // Add a song to the playlist
    public void addSong(String title) {
        Song newSong = new Song(title);
        if (head == null) {
            head = newSong;
            head.next = head;
            head.prev = head;
            current = head;
        } else {
            Song tail = head.prev;
            tail.next = newSong;
            newSong.prev = tail;
            newSong.next = head;
            head.prev = newSong;
        }
        System.out.println("Added: " + title);
    }

    // Remove a song by title
    public void removeSong(String title) {
        if (head == null) {
            System.out.println("Playlist is empty.");
            return;
        }

        Song temp = head;
        boolean found = false;

        do {
            if (temp.title.equalsIgnoreCase(title)) {
                found = true;
                if (temp == head && temp.next == head) {
                    head = null;
                    current = null;
                } else {
                    temp.prev.next = temp.next;
                    temp.next.prev = temp.prev;
                    if (temp == head) head = temp.next;
                    if (temp == current) current = temp.next;
                }
                System.out.println("Removed: " + title);
                break;
            }
            temp = temp.next;
        } while (temp != head);

        if (!found)
            System.out.println("Song not found: " + title);
    }

    // Search for a song
    public void searchSong(String title) {
        if (head == null) {
            System.out.println("Playlist is empty.");
            return;
        }

        Song temp = head;
        do {
            if (temp.title.equalsIgnoreCase(title)) {
                System.out.println("Found: " + title);
                return;
            }
            temp = temp.next;
        } while (temp != head);

        System.out.println("Song not found: " + title);
    }

    // Play the current song
    public void playCurrent() {
        if (current == null)
            System.out.println("Playlist is empty.");
        else
            System.out.println("Now playing: " + current.title);
    }

    // Next song
    public void nextSong() {
        if (current != null) {
            current = current.next;
            playCurrent();
        }
    }

    // Previous song
    public void previousSong() {
        if (current != null) {
            current = current.prev;
            playCurrent();
        }
    }

    // Show playlist
    public void showPlaylist() {
        if (head == null) {
            System.out.println("Playlist is empty.");
            return;
        }

        System.out.print("Playlist: ");
        Song temp = head;
        do {
            System.out.print(temp.title + " -> ");
            temp = temp.next;
        } while (temp != head);
        System.out.println("(back to start)");
    }
}

public class Main {
    public static void main(String[] args) {
        Playlist playlist = new Playlist();

        playlist.addSong("Shape of You");
        playlist.addSong("Blinding Lights");
        playlist.addSong("Levitating");
        playlist.addSong("Peaches");

        System.out.println();
        playlist.showPlaylist();

        System.out.println();
        playlist.playCurrent();
        playlist.nextSong();
        playlist.previousSong();

        System.out.println();
        playlist.searchSong("Levitating");
        playlist.removeSong("Blinding Lights");

        System.out.println();
        playlist.showPlaylist();
    }
}
