import { useEffect } from "react";
import "./App.css";
import axios from "axios";

const BASE_URL = "http://localhost:3005";
axios.defaults.baseURL = BASE_URL;

function App() {
  const getAllUsers = async () => {
    try {
      const response = await axios.get(BASE_URL + "/users");
      console.log(response.data);
    } catch (error) {
      console.error("Error fetching users:", error);
    }
  };

  const getUserById = async (id) => {
    try {
      const response = await axios.get(`${BASE_URL}/users/${id}`);
      console.log(response.data);
    } catch (error) {
      console.error(`Error fetching user with ID ${id}:`, error);
    }
  };
  const createUser = async (user) => {
    try {
      const response = await axios.post(BASE_URL + "/users", user);
      console.log("User created:", response.data);
    } catch (error) {
      console.error("Error creating user:", error);
    }
  };
  const updateUser = async (id, user) => {
    try {
      const response = await axios.put(`${BASE_URL}/users/${id}`, user);
      console.log("User updated:", response.data);
    } catch (error) {
      console.error(`Error updating user with ID ${id}:`, error);
    }
  };
  const deleteUser = async (id) => {
    try {
      const response = await axios.delete(`${BASE_URL}/users/${id}`);
      console.log("User deleted:", response.data);
    } catch (error) {
      console.error(`Error deleting user with ID ${id}:`, error);
    }
  };

  useEffect(() => {
    getAllUsers();
    getUserById("a3ab");
    createUser({ username: "elanto", password: "123456" });
    updateUser("57ce", { username: "elanto", password: "rini" });
    deleteUser("fb75");
  }, []);

  return <></>;
}

export default App;
