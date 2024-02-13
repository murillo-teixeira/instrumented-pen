struct MenuItem
{
  char key;
  const char *description;
  void (*action)();
};

extern MenuItem mainMenu[];
extern const size_t mainMenuSize;

void displayMenu()
{
  Serial.println("\n--------------------");
  for (size_t i = 0; i < mainMenuSize; ++i)
  {
    Serial.print("[");
    Serial.print(mainMenu[i].key);
    Serial.print("] ");
    Serial.println(mainMenu[i].description);
  }
  Serial.print("\nEnter your selection: ");
}

void readSelection()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    char selection = input.charAt(0);
    if (input.length() == 1 || selection == '\n')
    {
      displayMenu();
      return;
    }
    for (size_t i = 0; i < mainMenuSize; ++i)
    {
      if (selection == mainMenu[i].key)
      {
        mainMenu[i].action();
        return;
      }
    }
    Serial.println("Invalid selection. Please try again.");
    Serial.print("\nEnter your selection: ");
  }
}

void flushSerialBuffer()
{
  while (Serial.available() > 0)
  {
    Serial.read();
  }
}
