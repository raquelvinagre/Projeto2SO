# Restaurant Simulation Project


## Team Members

| Nmec   | Name           | Email                  | Github                                                |
| ------ | -------------- | ---------------------- | ----------------------------------------------------- |
| 114137 | Diogo Fernandes    | <diogomiguel.fernandes@ua.pt>    | [diogux](https://github.com/diogux)       |
| 113736 | Raquel Vinagre   | <raquelvinagre@ua.pt>     | [raquelvinagre](https://github.com/raquelvinagre)               |


## Relatório
[Relatório](https://uapt33090-my.sharepoint.com/:w:/g/personal/raquelvinagre_ua_pt/ERFVvgJHE9xAi_01TgKS4zQB-tiYRQYHlR1W98Hs4N5D9Q?e=phxAAL)


## Description

This project simulates a restaurant scenario where multiple groups need to dine. The restaurant consists of two tables, a receptionist, a waiter, and a chef. Groups approach the receptionist to get a table assignment, order food from the waiter, and pay the bill before leaving. The waiter coordinates between the groups and the chef, while the chef prepares the food.

## Implementation

Using provided C source code as a base, the task is to develop a program in C that simulates the restaurant scenario. Processes for groups, receptionist, waiter, and chef are created and synchronized using semaphores and shared memory. Processes should only be active when necessary and should block when waiting for an event. 

## Usage

To run the simulation, execute the following commands:

```bash
cd src
make all_bin
cd ../run
./probSemSharedMemRestaurant
