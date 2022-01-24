# Glossary

Here are some terms that we use with specific technical meanings, and will try to use consistently throughout code and documentation:

* **Generator** - a function or program such as e.g. Csmith which takes some source of randomness and produces some value, a **test case**.
* **Test case** - a value that is produced by a **generator**.
* **Chooser** (TODO: Better name) - core API object with a `choice` method, source of randomness.
* **Balancer** (TODO: Better name?) - core API object that acts as a source of **choosers**.
* **Choice sequence** - a sequence of integers corresponding to the results of some sequence of calls to `choice` by some **generator**.
* **Static choice point** - some point (file and line number) in a **generator**'s program where `choice` is called. 
